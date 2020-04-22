/*
BEGIN_TEST_DATA
  f: main
  ret: double
  args: double
  input: 12
  output: 29
  error: ""
  filename: "parameter/parameter_mixed"
END_TEST_DATA
*/

struct MyRangeConverter
{
	static double from0To1(double input)
	{
		return input * 2.0;
	}
};

struct OtherTest
{
	template <int P> void setParameter(double v)
	{
		if(P == 1)
			o = v * 9.0;
	}
	
	double o = 12.0;
};

struct Test
{
	template <int P> void setParameter(double v)
	{
		value = v;
	}
	
	double value = 12.0;
};


#define DECLARE_PARAMETER_EXPRESSION(name, expression) struct name { static double op(double input) { return expression; }}

DECLARE_PARAMETER_EXPRESSION(TestExpression,  input + 1.0);


using ParameterType1 = parameter::plain<Test, 0>;
using OtherParameter = parameter::expression<OtherTest, 1, TestExpression>;
using ParameterChainType = parameter::chain<MyRangeConverter, ParameterType1, OtherParameter>;


container::chain<ParameterChainType, Test, OtherTest> c;

double main(double input)
{
	auto& first = c.get<0>();
	auto& second = c.get<1>();
	
	auto& pc = c.getParameter<0>();

	pc.connect<0>(first);
	pc.connect<1>(second);

	c.setParameter<0>(0.2);

	return c.get<0>().value + c.get<1>().o;
}


