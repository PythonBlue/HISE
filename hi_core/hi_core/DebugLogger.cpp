
struct DebugLogger::Message
{
	Message() {};

	Message(int messageIndex_, int callbackIndex_, double timestamp_, Location l, Processor* const p_, const Identifier& id_) :
		messageIndex(messageIndex_),
		callbackIndex(callbackIndex_),
		timestamp(timestamp_),
		p(p_),
		id(id_),
		location(l)
	{};

	String getTimeString()
	{
		String t;
		NewLine nl;

		t << "- Time: **" << String(timestamp, 2) << "**  " << " / ";
		t << "CallbackIndex: **" << String(callbackIndex) << "**  " << nl;

		return t;
	}

	String getLocationString()
	{
		String l;
		NewLine nl;


		l << "- Location: `";

		if (p.get() != nullptr)
		{
			l << p.get()->getId() << "::";
		}

		if (!id.isNull())
		{
			l << id.toString() << "::";
		}

		l << DebugLogger::getNameForLocation(location) << "`  " << nl;

		return l;
	}

	virtual bool shouldPrintBacktrace() const { return false; };

	virtual String getMessageText(int errorIndex = -1) { ignoreUnused(errorIndex); jassertfalse; return String(); }


	int messageIndex = -1;

	int callbackIndex = -1;
	double timestamp = 0.0;

	const Identifier id;
	WeakReference<Processor> p;

	Location location;
};

struct DebugLogger::StringMessage : public DebugLogger::Message
{
	StringMessage(int messageIndex, int callbackIndex, const String& message_, double ts) :
		Message(messageIndex, callbackIndex, ts, Location::Empty, nullptr, Identifier()),
		message(message_)
	{}

	String getMessageText(int errorIndex /* = -1 */) override 
	{
		ignoreUnused(errorIndex);

		String s;
		NewLine nl;

		s << message << "(CI: `" << String(callbackIndex) << "`)  ";

		return s; 
	}

	String message;
};

struct DebugLogger::Event : public DebugLogger::Message
{
	Event(int messageIndex, int callbackIndex, const HiseEvent& e_) :
		Message(messageIndex, callbackIndex, timestamp, Location::MainRenderCallback, nullptr, Identifier()),
		e(e_)
	{}

	String getMessageText(int errorIndex /* = -1 */) override
	{
		ignoreUnused(errorIndex);

		String eventString;
		
		eventString << "**" << e.getTypeAsString() << "** CI: `" << String(callbackIndex) << "` ID: `" << String(e.getEventId()) << "` TS: `" << String(e.getTimeStamp()) << "` ";
		eventString << "V1: `" << (e.isNoteOnOrOff() ? MidiMessage::getMidiNoteName(e.getNoteNumber(), true, true, 3) : String(e.getNoteNumber())) << "`, V2: `" << String(e.getVelocity()) << "`, Ch: `" << String(e.getChannel()) << "`  ";

		return eventString;
	}

	const HiseEvent e;
};

struct DebugLogger::AudioSettingChange : public DebugLogger::Message
{
	AudioSettingChange(int messageIndex, int callbackIndex, double ts, FailureType type_, double oldValue_, double newValue_) :
		Message(messageIndex, callbackIndex, ts, Location::MainRenderCallback, nullptr, Identifier()),
		type(type_),
		oldValue(oldValue_),
		newValue(newValue_)
	{};

	FailureType type;
	double oldValue;
	double newValue;

	String getMessageText(int errorIndex = -1)
	{
		ignoreUnused(errorIndex);
		
		NewLine nl;
		String errorMessage;

		errorMessage << "### " << DebugLogger::getNameForFailure(type) << nl;

		errorMessage << getTimeString();

		if (type == FailureType::SampleRateChange || type == FailureType::BufferSizeChange)
		{
			errorMessage << "- Old: **" << String(oldValue, 0) << "**  " << nl;
			errorMessage << "- New: **" << String(newValue, 0) << "**  " << nl << nl;
			return errorMessage;
		}
	}
};

struct DebugLogger::PerformanceWarning : public DebugLogger::Message
{
	PerformanceWarning(int messageIndex, int callbackIndex, const DebugLogger::PerformanceData& d_, double timestamp_, int voiceAmount_) :
		Message(messageIndex, callbackIndex, timestamp_, (Location)d_.location, d_.p, Identifier()),
		d(d_),
		voiceAmount(voiceAmount_),
		timestamp(timestamp_)
	{};

	int voiceAmount = 0;
	double timestamp = 0.0;

	String getMessageText(int /*errorIndex*/) override
	{
		String errorMessage;
		NewLine nl;

		errorMessage << "### PerformanceWarning" << nl;

		errorMessage << getTimeString();
		errorMessage << getLocationString();

		errorMessage << "- Voice Amount: **" << String(voiceAmount) << "**  " << nl;
		errorMessage << "- Limit: `" << String(100.0 * d.limit, 1) << "%` Avg: `" << String(d.averagePercentage, 2) << "%`, Peak: `" << String(d.thisPercentage, 1) << "%`  ";

		return errorMessage;
	}

	const DebugLogger::PerformanceData d;
};

struct DebugLogger::ParameterChange : public DebugLogger::Message
{
	ParameterChange(int messageIndex, int callbackIndex, double timestamp, const Identifier& id, const var& value_):
		Message(messageIndex, callbackIndex, timestamp, Location::Empty, nullptr, id),
		value(value_)
	{}

	ParameterChange() :
		value(var())
	{};

	String getMessageText(int) override
	{
		String s;
		NewLine nl;

		s << "**Parameter Change** ";
		s << "ID: `" << id << "` value: `" << value.toString() << "`  " << "CI: `" << callbackIndex << "`  ";
		return s;
	}

	const var value;
};

struct DebugLogger::Failure : public DebugLogger::Message
{
	Failure(int messageIndex, int callbackIndex_, Location loc_, FailureType t, Processor* faultyModule, double ts, double extraValue_, const Identifier& id_ = Identifier()) :
		Message(messageIndex, callbackIndex_, ts, loc_, faultyModule, id_),
		type(t),
		extraValue(extraValue_)
	{};

	bool shouldPrintBacktrace() const override { return type == FailureType::PriorityInversion; }

	String getMessageText(int errorIndex = -1)
	{
		static const String ok("All OK");

		if (type == FailureType::Empty)
			return ok;

		NewLine nl;
		String errorMessage;

		if (errorIndex == -1)
		{
			errorMessage << "### " << DebugLogger::getNameForFailure(type) << nl;
		}
		else
		{
			errorMessage << "### #" << String(errorIndex) << ": " << DebugLogger::getNameForFailure(type) << nl;
		}

		errorMessage << getTimeString();

		errorMessage << getLocationString();

		if (extraValue != 0.0)
		{
			errorMessage << "- AdditionalInfo: **" << String(extraValue, 3) << "**  " << nl;
		}


		errorMessage << nl;
		return errorMessage;
	}

	FailureType type;
	
	double extraValue = 0.0;
	
};


DebugLogger::DebugLogger(MainController* mc_):
	mc(mc_)
{
	pendingEvents.ensureStorageAllocated(NUM_MESSAGE_SLOTS);
	pendingFailures.ensureStorageAllocated(NUM_MESSAGE_SLOTS);
	pendingPerformanceWarnings.ensureStorageAllocated(NUM_MESSAGE_SLOTS);
	pendingStringMessages.ensureStorageAllocated(NUM_MESSAGE_SLOTS);
	pendingAudioChanges.ensureStorageAllocated(16);
}

DebugLogger::~DebugLogger()
{

}

double DebugLogger::getCurrentTimeStamp() const
{
	return 0.001 * (Time::getMillisecondCounterHiRes() - uptime);
}

void DebugLogger::addFailure(const DebugLogger::Failure& f)
{
	ScopedLock sl(debugLock);
	pendingFailures.add(f);
}

void DebugLogger::addPerformanceWarning(const PerformanceWarning& f)
{
	ScopedLock sl(debugLock);
	pendingPerformanceWarnings.add(f);
}


void DebugLogger::addStreamingFailure(double voiceUptime)
{
	Failure f = Failure(messageIndex++, callbackIndex, Location::SampleRendering, FailureType::StreamingFailure, nullptr, getCurrentTimeStamp(), voiceUptime);

	addFailure(f);
}

void DebugLogger::logEvents(const HiseEventBuffer& masterBuffer)
{
	if (isLogging())
	{
		HiseEventBuffer::Iterator iter(masterBuffer);

		while (auto e = iter.getNextConstEventPointer())
		{
			if (e->isAftertouch())
				continue;

			ScopedLock sl(debugLock);

			Event e2(messageIndex++, callbackIndex, *e);

			pendingEvents.add(e2);
		}
	}
}

void DebugLogger::logMessage(const String& errorMessage)
{
	ScopedLock sl(messageLock);

	StringMessage m(messageIndex++, callbackIndex, errorMessage, getCurrentTimeStamp());

	pendingStringMessages.add(m);
}

void DebugLogger::logPerformanceWarning(const PerformanceData& logData)
{
	if (!isLogging())
		return;

	const int voiceAmount = logData.p->getMainController()->getNumActiveVoices();

	PerformanceWarning f(messageIndex++, callbackIndex, logData, getCurrentTimeStamp(), voiceAmount);
	addPerformanceWarning(f);
}


void DebugLogger::logParameterChange(JavascriptProcessor* p, ReferenceCountedObject* control, const var& newValue)
{
	if (isLogging() && control != nullptr)
	{
		if (auto jmp = dynamic_cast<JavascriptMidiProcessor*>(p))
		{
			if (jmp->isFront())
			{
				auto c = dynamic_cast<ScriptingApi::Content::ScriptComponent*>(control);

				Identifier id = c->getName();

				DebugLogger::ParameterChange pc(messageIndex++, callbackIndex, getCurrentTimeStamp(), id, newValue);

				ScopedLock sl(debugLock);

				if (pendingParameterChanges.getLast().id == id)
				{
					pendingParameterChanges.removeLast();
				}
				
				pendingParameterChanges.add(pc);
			}
		}
	}
}

void DebugLogger::checkAudioCallbackProperties(double sampleRate, int samplesPerBlock)
{
	if (!isLogging()) return;

	callbackIndex++;

	locationForErrorInCurrentCallback = Location::Empty;

	if (sampleRate != lastSampleRate)
	{
		addAudioDeviceChange(FailureType::SampleRateChange, lastSampleRate, sampleRate);
		lastSampleRate = sampleRate;
	}

	if (samplesPerBlock != lastSamplesPerBlock)
	{
		addAudioDeviceChange(FailureType::BufferSizeChange, lastSamplesPerBlock, samplesPerBlock);
		lastSamplesPerBlock = samplesPerBlock;
	}
}

bool DebugLogger::checkSampleData(Processor* p, Location location, bool isLeftChannel, const float* data, int numSamples, const Identifier& id/*=Identifier()*/)
{
	if (!isLogging()) return true;

	//if (location <= locationForErrorInCurrentCallback) // poor man's stack trace
	//	return true;

	auto range = FloatVectorOperations::findMinAndMax(data, numSamples);

	const float maxValue = 32.0f;

	double errorValue = 0.0;
	bool isError = false;
	int numFaultySamples = 0;

	FailureType failureType = FailureType::Empty;

	if (range.getEnd() > maxValue)
	{
		isError = true;

		errorValue = range.getEnd();

		for (int i = 0; i < numSamples; i++)
		{
			if (data[i] > maxValue)
				numFaultySamples++;
		}
	}

	if (range.getStart() < -maxValue)
	{
		isError = true;

		errorValue = range.getStart();

		for (int i = 0; i < numSamples; i++)
		{
			if (data[i] < -maxValue)
				numFaultySamples++;
		}
	}

	if (isError)
	{
		locationForErrorInCurrentCallback = location;

		if (numFaultySamples == 1)
		{
			failureType = isLeftChannel ? FailureType::ClickLeft : FailureType::ClickRight;
		}
		else
		{
			failureType = isLeftChannel ? FailureType::BurstLeft : FailureType::BurstRight;
		}

		Failure f(messageIndex++, callbackIndex, location, failureType, p, getCurrentTimeStamp(), errorValue, id);
		addFailure(f);

		return false;
	}

	return true;
}

void DebugLogger::checkAssertion(Processor* p, Location location, bool result, double extraData)
{
	if (!isLogging()) return;

	if (!result)
	{
		Failure f(messageIndex++, callbackIndex, location, FailureType::Assertion, p, getCurrentTimeStamp(), extraData);
		addFailure(f);
	}
}


void DebugLogger::checkPriorityInversion(const CriticalSection& lockToCheck)
{
	if (!isLogging())
		return;

	ScopedTryLock sl(lockToCheck);

	if (!sl.isLocked())
	{
		Failure f(messageIndex++, callbackIndex - 1, Location::MainRenderCallback, FailureType::PriorityInversion, nullptr, getCurrentTimeStamp(), 0.0);

		actualBackTrace = messageCallbackStackBacktrace;

		addFailure(f);
	}
}

void DebugLogger::checkPriorityInversion(const SpinLock& spinLockToCheck, Location l, Processor* p, const Identifier& id)
{
	if (!isLogging())
		return;

	bool success = spinLockToCheck.tryEnter();

	if (success)
		spinLockToCheck.exit();
	else
	{
		Failure f(messageIndex++, callbackIndex, l, FailureType::PriorityInversion, p, getCurrentTimeStamp(), 0.0, id);
		addFailure(f);
	}
}

void DebugLogger::addAudioDeviceChange(FailureType changeType, double oldValue, double newValue)
{
	if (isLogging())
	{
		AudioSettingChange f(messageIndex++, callbackIndex, getCurrentTimeStamp(), changeType, oldValue, newValue);
		{
			ScopedLock sl(debugLock);
			pendingAudioChanges.add(f);
		}
	}
}

void DebugLogger::startLogging()
{
	currentLogFile = getLogFile();
	currentlyLogging = true;
	currentLogFile.create();
	numErrorsSinceLogStart = 0;

	lastSampleRate = -1.0;
	lastSamplesPerBlock = -1;

	callbackIndex = 0;

	uptime = Time::getMillisecondCounterHiRes();

	FileOutputStream fos(currentLogFile);

	fos << getHeader();
	fos << getSystemSpecs();
	

	pendingFailures.ensureStorageAllocated(200);

	startTimer(200);

	for (int i = 0; i < listeners.size(); i++)
	{
		if (listeners[i].get() != nullptr)
		{
			listeners[i]->logStarted();
		}
	}
}


struct MessageComparator
{
	static int compareElements(DebugLogger::Message* first, DebugLogger::Message* second)
	{
		if (first->messageIndex > second->messageIndex)
		{
			return 1;
		}
		else
			return -1;
	}
};


void DebugLogger::timerCallback()
{
	Array<Failure> failureCopy;
	Array<StringMessage> messageCopy;
	Array<PerformanceWarning> warningCopy;
	Array<Event> eventCopy;
	Array<AudioSettingChange> audioCopy;
	Array<ParameterChange> parameterCopy;

	{
		if (pendingFailures.size() != 0)
		{
			failureCopy.ensureStorageAllocated(pendingFailures.size());

			ScopedLock sl(debugLock);
			failureCopy.addArray(pendingFailures);
			pendingFailures.clearQuick();
		}

		if (pendingStringMessages.size() != 0)
		{
			messageCopy.ensureStorageAllocated(pendingStringMessages.size());

			ScopedLock sl(messageLock);
			messageCopy.addArray(pendingStringMessages);
			pendingStringMessages.clearQuick();
		}

		if (pendingEvents.size() != 0)
		{
			eventCopy.ensureStorageAllocated(pendingEvents.size());

			ScopedLock sl(debugLock);
			eventCopy.addArray(pendingEvents);
			pendingEvents.clearQuick();
		}

		if (pendingPerformanceWarnings.size() != 0)
		{
			warningCopy.ensureStorageAllocated(pendingPerformanceWarnings.size());

			ScopedLock sl(debugLock);
			warningCopy.addArray(pendingPerformanceWarnings);
			pendingPerformanceWarnings.clearQuick();
		}

		if (pendingAudioChanges.size() != 0)
		{
			audioCopy.ensureStorageAllocated(pendingAudioChanges.size());

			ScopedLock sl(debugLock);
			audioCopy.addArray(pendingAudioChanges);
			pendingAudioChanges.clearQuick();
		}

		if (pendingParameterChanges.size() != 0)
		{
			parameterCopy.ensureStorageAllocated(pendingParameterChanges.size());

			ScopedLock sl(debugLock);
			parameterCopy.addArray(pendingParameterChanges);
			pendingParameterChanges.clearQuick();
		}
	}

	messageIndex = 0;

	Array<Message*> messages;

	

	for (int i = 0; i < warningCopy.size(); i++)
		messages.add(&warningCopy.getReference(i));
	
	for (int i = 0; i < eventCopy.size(); i++)
		messages.add(&eventCopy.getReference(i));

	for (int i = 0; i < failureCopy.size(); i++)
		messages.add(&failureCopy.getReference(i));

	for (int i = 0; i < messageCopy.size(); i++)
		messages.add(&messageCopy.getReference(i));

	for (int i = 0; i < audioCopy.size(); i++)
		messages.add(&audioCopy.getReference(i));

	for (int i = 0; i < parameterCopy.size(); i++)
		messages.add(&parameterCopy.getReference(i));

	if (!messages.isEmpty())
	{
		FileOutputStream fos(currentLogFile, 512);
		NewLine nl;


		MessageComparator mec;
		messages.sort(mec, false);

		for (auto m : messages)
		{
			fos << m->getMessageText() << nl;

			if (m->shouldPrintBacktrace() && actualBackTrace.isNotEmpty())
			{
				fos << "#### Stack back trace" << nl << nl;
				fos << "```" << nl;

				fos << actualBackTrace;
				fos << "```" << nl << nl;
				actualBackTrace = String();
			}
		}

		const bool containsError = warningCopy.size() != 0 || failureCopy.size() != 0;

		if (containsError)
		{

			for (int i = 0; i < listeners.size(); i++)
			{
				if (listeners[i].get() != nullptr)
					listeners[i]->errorDetected();
			}

			numErrorsSinceLogStart += failureCopy.size();
			numErrorsSinceLogStart += warningCopy.size();
		}
	}
	else
	{
		currentlyFailing = false;
	}

	



#if 0
	if (messages.size() != 0)
	{
		FileOutputStream fos(currentLogFile, 512);
		NewLine nl;

		ScopedLock sl(messageLock);

		for (auto m : messages)
		{
			fos << "#### " << m << "  " << nl;
			fos << "- Callback Index: **" << String(callbackIndex) << "**  " << nl << nl;
		}

		messages.clear();
		callbackIndexesForMessage.clear();
	}

	

	Array<Failure> a;

	if ( !pendingFailures.isEmpty())
	{
		FileOutputStream fos(currentLogFile, 512);

		a.ensureStorageAllocated(pendingFailures.size());

		{
			ScopedLock sl(debugLock);
			a.addArray(pendingFailures);
			pendingFailures.clearQuick();
		}

		

		for (int i = 0; i < a.size(); i++)
		{
			fos << a[i].getMessageText(numErrorsSinceLogStart);

			

			numErrorsSinceLogStart++;
		}
		
		lastErrorMessage = getNameForFailure(a.getLast().type);

		currentlyFailing = true;

		for (int i = 0; i < listeners.size(); i++)
		{
			if (listeners[i].get() != nullptr)
				listeners[i]->errorDetected();
		}

		return;
	}
	else
	{
		currentlyFailing = false;
	}

	if (!pendingMessages.isEmpty())
	{
		FileOutputStream fos(currentLogFile, 512);

		a.clearQuick();

		a.ensureStorageAllocated(pendingMessages.size());

		{
			ScopedLock sl(debugLock);
			a.addArray(pendingMessages);
			pendingMessages.clearQuick();
		}

		for (int i = 0; i < a.size(); i++)
			fos << a[i].getMessageText();
	}
#endif
}

bool DebugLogger::isLogging() const
{
	return currentlyLogging && numErrorsSinceLogStart < 200;
}

void DebugLogger::stopLogging()
{
	currentlyLogging = false;
	stopTimer();

	for (int i = 0; i < listeners.size(); i++)
	{
		if (listeners[i].get() != nullptr)
		{
			listeners[i]->logEnded();
		}
	}
}

void DebugLogger::toggleLogging()
{
	if (isLogging()) stopLogging();
	else			 startLogging();
}

bool DebugLogger::isCurrentlyFailing() const
{
	return currentlyFailing;
}

void DebugLogger::addListener(Listener* newListener)
{
	listeners.addIfNotAlreadyThere(newListener);
}

void DebugLogger::removeListener(Listener* listenerToRemove)
{
	listeners.removeAllInstancesOf(listenerToRemove);
}

String DebugLogger::getLastErrorMessage() const
{
	String rs;

	rs << "# Errors: " << String(numErrorsSinceLogStart) << ", Last Error Type: " << lastErrorMessage;

	return rs;
}

void DebugLogger::showLogFolder()
{
	getLogFolder().revealToUser();
}

#define RETURN_CASE_STRING_LOCATION(x) case DebugLogger::Location::x: return #x

String DebugLogger::getNameForLocation(Location l)
{
	switch (l)
	{
		RETURN_CASE_STRING_LOCATION(Empty);
		RETURN_CASE_STRING_LOCATION(MainRenderCallback);
		RETURN_CASE_STRING_LOCATION(MultiMicSampleRendering);
		RETURN_CASE_STRING_LOCATION(SampleRendering);
		RETURN_CASE_STRING_LOCATION(ScriptFXRendering);
		RETURN_CASE_STRING_LOCATION(ScriptFXRenderingPost);
		RETURN_CASE_STRING_LOCATION(DspInstanceRendering);
		RETURN_CASE_STRING_LOCATION(DspInstanceRenderingPost);
		RETURN_CASE_STRING_LOCATION(TimerCallback);
		RETURN_CASE_STRING_LOCATION(SampleLoaderPreFillVoiceBufferRead);
		RETURN_CASE_STRING_LOCATION(SampleLoaderPreFillVoiceBufferWrite);
		RETURN_CASE_STRING_LOCATION(SampleLoaderPostFillVoiceBuffer);
		RETURN_CASE_STRING_LOCATION(SampleLoaderPostFillVoiceBufferWrapped);
		RETURN_CASE_STRING_LOCATION(SampleVoiceBufferFill);
		RETURN_CASE_STRING_LOCATION(SampleVoiceBufferFillPost);
		RETURN_CASE_STRING_LOCATION(SampleLoaderReadOperation);
		RETURN_CASE_STRING_LOCATION(SynthRendering);
		RETURN_CASE_STRING_LOCATION(SynthPreVoiceRendering);
		RETURN_CASE_STRING_LOCATION(SynthPostVoiceRenderingGainMod);
		RETURN_CASE_STRING_LOCATION(SynthPostVoiceRendering);
		RETURN_CASE_STRING_LOCATION(SynthChainRendering);
		RETURN_CASE_STRING_LOCATION(SampleStart);
		RETURN_CASE_STRING_LOCATION(VoiceEffectRendering);
		RETURN_CASE_STRING_LOCATION(ModulatorChainVoiceRendering);
		RETURN_CASE_STRING_LOCATION(ModulatorChainTimeVariantRendering);
		RETURN_CASE_STRING_LOCATION(SynthVoiceRendering);
		RETURN_CASE_STRING_LOCATION(NoteOnCallback);
		RETURN_CASE_STRING_LOCATION(NoteOffCallback);
		RETURN_CASE_STRING_LOCATION(MasterEffectRendering);
		RETURN_CASE_STRING_LOCATION(ScriptMidiEventCallback);
		RETURN_CASE_STRING_LOCATION(ConvolutionRendering);
	}

	return "Unknown Location";

}
	
#undef RETURN_CASE_STRING_LOCATION

#define RETURN_CASE_STRING_FAILURE(x) case DebugLogger::FailureType::x: return #x

String DebugLogger::getNameForFailure(FailureType f)
{
	switch (f)
	{
		RETURN_CASE_STRING_FAILURE(Empty);
		RETURN_CASE_STRING_FAILURE(SampleRateChange);
		RETURN_CASE_STRING_FAILURE(Assertion);
		RETURN_CASE_STRING_FAILURE(BufferSizeChange);
		RETURN_CASE_STRING_FAILURE(PerformanceWarning);
		RETURN_CASE_STRING_FAILURE(BurstLeft);
		RETURN_CASE_STRING_FAILURE(BurstRight);
		RETURN_CASE_STRING_FAILURE(ClickLeft);
		RETURN_CASE_STRING_FAILURE(ClickRight);
		RETURN_CASE_STRING_FAILURE(AudioThreadWasLocked);
		RETURN_CASE_STRING_FAILURE(Discontinuity);
		RETURN_CASE_STRING_FAILURE(PriorityInversion);
		RETURN_CASE_STRING_FAILURE(SampleLoadingError);
		RETURN_CASE_STRING_FAILURE(StreamingFailure);
	}

	return "Unknown failure";
}

void DebugLogger::fillBufferWithJunk(float* data, int numSamples)
{
	Random r;

	bool shouldFillWithBurst = r.nextFloat() > 0.992f;
	bool shouldFillWithClick = r.nextFloat() > 0.992f;
	bool shouldAddInf = r.nextFloat() > 0.992f;
	bool shouldAddNaN = r.nextFloat() > 0.992f;

	if (shouldFillWithBurst)
	{
		for (int i = 0; i < numSamples; i++)
		{
			data[i] = (2.0f * r.nextFloat() - 1.0f) * FLT_MAX;
		}

		return;
	}

	if (shouldFillWithClick)
	{
		data[0] = (2.0f * r.nextFloat() - 1.0f) * FLT_MAX;

		return;
	}

	if (shouldAddInf)
	{
		data[0] = INFINITY;
		return;
	}

	if (shouldAddNaN)
	{
		data[0] = NAN;
		return;
	}
}

void DebugLogger::setPerformanceWarningLevel(int newWarningLevel)
{
	logMessage("New Warning level selected: " + String(newWarningLevel));

	warningLevel = newWarningLevel;
}



#undef RETURN_CASE_STRING_FAILURE

File DebugLogger::getLogFile()
{
	File f = getLogFolder().getChildFile("Debuglog.txt");

	return f.getNonexistentSibling();
}

File DebugLogger::getLogFolder()
{
#if USE_BACKEND

	File f = File(PresetHandler::getDataFolder()).getChildFile("Logs/");

#else

	File f = ProjectHandler::Frontend::getAppDataDirectory().getChildFile("Logs/");

#endif

	if (!f.isDirectory())
		f.createDirectory();

	return f;
}

String DebugLogger::getHeader()
{
	String header;
	NewLine nl;

	header << "# Debug Log file\n\n" << nl;
#if USE_BACKEND
	header << "Product: **HISE**  " << nl;
	header << "Version: **" << ProjectInfo::versionString << "**  " << nl;
#else

	header << "Product: **" << ProjectHandler::Frontend::getCompanyName() << " - " << ProjectHandler::Frontend::getProjectName() << "**  " << nl;
	header << "Version: **" << ProjectHandler::Frontend::getVersionString() << "**  " << nl;

#endif

	header << "Time created: **" << Time::getCurrentTime().formatted("%d.%B %Y - %H:%M:%S") << "**  " << nl << nl;

	return header;
}


void DebugLoggerComponent::paint(Graphics& g)
{
	g.fillAll(isFailing ? Colours::red.withAlpha(0.8f) : Colours::black.withAlpha(0.8f));

	g.setColour(Colours::white.withAlpha(0.4f));

	g.drawLine(0.0f, 0.0f, (float)getWidth(), 0.0f, 2.0f);

	Rectangle<int> r = getLocalBounds();

	r.reduce(20, 20);
	r.setWidth(getWidth() - 200);

	g.setColour(Colours::white.withAlpha(0.1f));

	g.setFont(GLOBAL_BOLD_FONT().withHeight(40.0f));

	g.drawText("DEBUG LOG ENABLED", r, Justification::centred);

	g.setColour(Colours::white);
	g.setFont(GLOBAL_BOLD_FONT());

	g.drawText(logger->getLastErrorMessage(), r, Justification::centredLeft);

	g.drawText("Warning Level:", performanceLevelSelector->getX(), 5, 140, 20, Justification::centred);
}


String DebugLogger::getSystemSpecs() const
{
	NewLine nl;

	String stats = "## System specification\n\n";

	stats << "Device: **" << SystemStats::getDeviceDescription() << "**  " << nl;
	stats << "User name: **" << SystemStats::getFullUserName() << "**  " << nl;
	stats << "CPU vendor: **" << SystemStats::getCpuVendor() << "**  " << nl;
	stats << "CPU cores: **" << SystemStats::getNumCpus() << "**  " << nl;
	stats << "CPU speed: **" << SystemStats::getCpuSpeedInMegaherz() << "**  " << nl;
	stats << "Memory size: **" << SystemStats::getMemorySizeInMegabytes() << "**  " << nl;
	stats << "Page size: **" << SystemStats::getPageSize() << "**  " << nl;
	stats << "OS: **" << SystemStats::getOperatingSystemName() << (SystemStats::isOperatingSystem64Bit() ? " 64bit" : " 32bit") << "**  " << nl;

#if !(IS_STANDALONE_APP || IS_STANDALONE_FRONTEND)

	juce::PluginHostType hostType;

	auto pt = hostType.getPluginLoadedAs();

	if (pt == AudioProcessor::wrapperType_AudioUnit)
	{
		stats << "Plugin Format: **AU**  " << nl;
	}
	else if (pt == AudioProcessor::wrapperType_AAX)
	{
		stats << "Plugin Format: **AAX**  " << nl;
	}
	else if (pt == AudioProcessor::wrapperType_VST)
	{
		stats << "Plugin Format: **VST**  " << nl;
	}

	hostType.getHostDescription();

	stats << "Host: **" << hostType.getHostDescription() << "**  " << nl;
	stats << "Host Path: **" << hostType.getHostPath() << "**  " << nl;
	
#else
	stats << "Host: **Standalone**  " << nl;
#endif

#if JUCE_64BIT
	stats << "Process bit architecture: **64 bit**  " << nl;
#else
	stats << "Process bit architecture: **32 bit**  " << nl;
#endif
	stats << "Sandboxed: **" << (SystemStats::isRunningInAppExtensionSandbox() ? " Yes" : " No") << "**  " << nl;
	
	stats << nl;

	return stats;
}

void DebugLogger::PriorityInversionChecker::preCallbackEvent()
{
	dynamic_cast<MainController*>(p)->getDebugLogger().checkPriorityInversion(p->getCallbackLock());
}
