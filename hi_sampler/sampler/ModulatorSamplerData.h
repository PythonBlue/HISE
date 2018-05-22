/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef MODULATORSAMPLERDATA_H_INCLUDED
#define MODULATORSAMPLERDATA_H_INCLUDED

namespace hise { using namespace juce;

class ModulatorSampler;
class ModulatorSamplerSound;



struct SampleMapData
{
	ValueTree data;
};

/** A SampleMap is a data structure that encapsulates all data loaded into an ModulatorSampler. 
*	@ingroup sampler
*
*	It saves / loads all sampler data (modulators, effects) as well as all loaded sound files.
*
*	It supports two saving modes (monolithic and file-system based).
*	It only accesses the sampler data when saved or loaded, and uses a ChangeListener to check if a sound has changed.
*/
class SampleMap: public SafeChangeListener,
				 public PoolBase::Listener,
				 public ValueTree::Listener
{
public:


	class Listener
	{
	public:

		virtual ~Listener() {};

		virtual void sampleMapWasChanged(PoolReference newSampleMap) = 0;

		virtual void samplePropertyWasChanged(ModulatorSamplerSound* s, const Identifier& id, const var& newValue) = 0;

		virtual void sampleAmountChanged() = 0;

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	/** A SamplerMap can be saved in multiple modes. */
	enum SaveMode
	{
		/** The default mode, until the map gets saved. */
		Undefined = 0,
		/** Saves all data using this file structure:
		*
		*	- the sample map will be saved as .xml file
		*	- the thumbnail data will be saved as thumbnail.dat
		*	- the samples will be saved into a '/samples' subfolder and replaced by relative file references.
		*	- the sampler data (modulators) will be stored as preset file (*.hip) containing a reference to the samplerMap
		*/
		MultipleFiles,
		/** Saves everything into a big file which contains all data. */
		Monolith,
		/** Saves everything into a big file and encrypts the header data using a RSA Key 
		*	which can be used to handle serial numbers
		*/
		MonolithEncrypted,
		numSaveModes
	};
	
	SampleMap(ModulatorSampler *sampler_);

	using FileList = OwnedArray < Array<File> >;

	FileList createFileList();

	~SampleMap()
	{
		if(!monolith) saveIfNeeded();
	};

	/** Checks if the samplemap was changed and deletes it. */
	void saveIfNeeded();

	void changeListenerCallback(SafeChangeBroadcaster *b);

	/** Checks if any ModulatorSamplerSound was changed since the last save. 
	*
	*	This feature is currently disabled.
	*	It does not check if any other ModulatorSampler Properties were changed.
	*/
	bool hasUnsavedChanges() const
	{
		return false; //fileOnDisk == File() || changed;
	}

	void load(const PoolReference& reference);

	void loadUnsavedValueTree(const ValueTree& v);

	/** Saves all data with the mode depending on the file extension. */
	void save();

	void saveAsMonolith(Component* mainEditor);

	void setIsMonolith() noexcept { mode = SaveMode::Monolith; }

	bool isMonolith() const noexcept { return mode == SaveMode::Monolith; };

	/** Clears the sample map. */
    void clear(NotificationType n);
	
	ModulatorSampler* getSampler() const { return sampler; }
	
	void setId(Identifier newIdentifier)
    {
        sampleMapId = newIdentifier.toString();
		data.setProperty("ID", sampleMapId.toString(), nullptr);
    }
    
    Identifier getId() const { return sampleMapId; };
    
	static String checkReferences(MainController* mc, ValueTree& v, const File& sampleRootFolder, Array<File>& sampleList);

	void addSound(ValueTree& newSoundData);

	void removeSound(ModulatorSamplerSound* s);

	/** Exports the SampleMap as ValueTree.
	*
	*	If the relative mode is enabled, it writes the files to the subdirectory '/samples',
	*	if they don't exist yet.
	*/
	const ValueTree getValueTree() const;

	PoolReference getReference() const
	{
		return sampleMapData.getRef();
	}

	void poolEntryReloaded(PoolReference referenceThatWasChanged) override;

	bool isUsingUnsavedValueTree() const
	{
		return !sampleMapData && data.getNumChildren() != 0;
	}

	void addListener(Listener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeListener(Listener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	void sendSampleMapChangeMessage(NotificationType n=sendNotificationAsync)
	{
		notifier.sendMapChangeMessage(n);
	}

	void valueTreePropertyChanged(ValueTree& /*treeWhosePropertyHasChanged*/,
		const Identifier& /*property*/);

	void valueTreeChildAdded(ValueTree& parentTree,
		ValueTree& childWhichHasBeenAdded) override;;

	void valueTreeChildRemoved(ValueTree& parentTree,
		ValueTree& childWhichHasBeenRemoved,
		int indexFromWhichChildWasRemoved) override;;

	void valueTreeChildOrderChanged(ValueTree& /*parentTreeWhoseChildrenHaveMoved*/,
		int /*oldIndex*/, int /*newIndex*/) override {};

	void valueTreeParentChanged(ValueTree& /*treeWhoseParentHasChanged*/) override {};

	void valueTreeRedirected(ValueTree& /*treeWhichHasBeenChanged*/) override {};

	ModulatorSamplerSound* getSound(int index);
	const ModulatorSamplerSound* getSound(int index) const;

	int getNumRRGroups() const;

private:

	void setCurrentMonolith();

	struct Notifier : private Timer,
					  private AsyncUpdater
	{
		struct AsyncPropertyChange
		{
			AsyncPropertyChange(ModulatorSamplerSound* sound, const Identifier& id, const var& newValue);

			bool operator==(const Identifier& id_) const
			{
				return id == id_;
			}

			Array<SynthesiserSound::Ptr> selection;
			Array<var> values;

			Identifier id;

			void addPropertyChange(ModulatorSamplerSound* sound, const var& newValue);
			
		};

		struct PropertyChange
		{
			bool operator==(int indexToCompare) const
			{
				return indexToCompare == index;
			}

			void set(const Identifier& id, const var& newValue)
			{
				propertyChanges.set(id, newValue);
			}

			

			int index;

			NamedValueSet propertyChanges;
		};

		Notifier(SampleMap& parent_):
			parent(parent_)
		{}

		void sendMapChangeMessage(NotificationType n)
		{
			sampleAmountWasChanged = false;
			mapWasChanged = true;

			if (n == sendNotificationAsync)
				triggerLightWeightUpdate();
			else
				handleLightweightPropertyChanges();
		}

		void addPropertyChange(int index, const Identifier& id, const var& newValue);

		void sendSampleAmountChangeMessage(NotificationType n)
		{
			sampleAmountWasChanged = true;

			if (n == sendNotificationAsync)
				triggerLightWeightUpdate();
			else
				handleLightweightPropertyChanges();
		}

	private:

		void handleHeavyweightPropertyChanges();

		void handleLightweightPropertyChanges();

		void triggerHeavyweightUpdate()
		{
			startTimer(100);
		}

		void triggerLightWeightUpdate()
		{
			triggerAsyncUpdate();
		}

		void handleAsyncUpdate();
		

		void timerCallback() override;


		Array<PropertyChange> pendingChanges;

		Array<AsyncPropertyChange> asyncPendingChanges;

		bool mapWasChanged = false;
		bool sampleAmountWasChanged = false;
		SampleMap& parent;
	};

	Notifier notifier;

	/** Restores the samplemap from the ValueTree.
	*
	*	If the files are saved in relative mode, the references are replaced
	*	using the parent directory of the sample map before they are loaded.
	*	If the files are saved as monolith, it assumes the files are already loaded and simply adds references to this samplemap.
	*/
	void parseValueTree(const ValueTree &v);

	PooledSampleMap sampleMapData;

	ValueTree data;

	void setNewValueTree(const ValueTree& v);

	ModulatorSampler *sampler;

	SaveMode mode;

	bool changed;
	bool monolith;

	WeakReference<SampleMapPool> currentPool;

	Array<WeakReference<Listener>> listeners;

	HlacMonolithInfo::Ptr currentMonolith;

    Identifier sampleMapId;
    
	JUCE_DECLARE_WEAK_REFERENCEABLE(SampleMap);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleMap)
		
};

/** A data container which stores information about the amount of round robin groups for each notenumber / velocity combination.
*
*	The information is precalculated so that the query is a very fast look up operation (O(1)). In order to use it, create one, and
*	call addSample() for every ModulatorSamplerSound you need.
*	You can query the rr group later with getRRGroupsForMessage().
*/
class RoundRobinMap
{
public:

	RoundRobinMap();

	/** Clears the map */
	void clear();

	/** adds the information of the sample to the map. It checks for every notenumber / velocity combination if it is the biggest group. */
	void addSample(const ModulatorSamplerSound *sample);

	/** returns the biggest group index for the given MIDI information. This is very fast. */
	int getRRGroupsForMessage(int noteNumber, int velocity);

private:

	char internalData[128][128];

};

#if HI_ENABLE_EXPANSION_EDITING
class MonolithExporter : public DialogWindowWithBackgroundThread,
						 public AudioFormatWriter
{
public:

	MonolithExporter(SampleMap* sampleMap_);

	MonolithExporter(const String &name, ModulatorSynthChain* chain);

	~MonolithExporter()
	{
		fc = nullptr;
	}

	static void collectFiles()
	{

	}

	void run() override;

	void exportCurrentSampleMap(bool overwriteExistingData, bool exportSamples, bool exportSampleMap);

	void setSampleMap(SampleMap* samplemapToExport)
	{
		sampleMap = samplemapToExport;
	}

	void writeSampleMapFile(bool overwriteExistingFile);

	void threadFinished() override;;

	bool write(const int** /*data*/, int /*numSamples*/) override
	{
		jassertfalse;
		return false;
	}
    
protected:
    
    File sampleMapFile;

private:

	void checkSanity();


	/** Writes the files and updates the samplemap with the information. */
	void writeFiles(int channelIndex, bool overwriteExistingData);

	void updateSampleMap();

	int64 largestSample;

	ScopedPointer<FilenameComponent> fc;

	ValueTree v;
	SampleMap* sampleMap;
	SampleMap::FileList filesToWrite;
	int numChannels;
	int numSamples;
	File sampleMapDirectory;
	const File monolithDirectory;
	

	String error;
};
#endif

} // namespace hise
#endif  // MODULATORSAMPLERDATA_H_INCLUDED
