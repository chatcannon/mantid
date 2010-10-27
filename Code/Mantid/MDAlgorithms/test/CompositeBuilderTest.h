#ifndef COMPOSITE_FUNCTION_BUILDER_TEST_H_
#define COMPOSITE_FUNCTION_BUILDER_TEST_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>
#include <vector>
#include <memory>
#include "CompositeFunctionBuilder.h"
#include "CompositeImplicitFunction.h"
#include "MantidAPI/ImplicitFunction.h"
#include "MantidAPI/ImplicitFunctionBuilder.h"
#include "MantidAPI/ImplicitFunctionParameter.h"
#include "MDDataObjects/point3D.h"

class CompositeBuilderTest : public CxxTest::TestSuite
{

private:

    class FakeParameter : public Mantid::API::ImplicitFunctionParameter
    {
    public:
        bool isValid() const
        {
            return false;
        }
		 MOCK_CONST_METHOD0(getName, std::string());
         MOCK_CONST_METHOD0(toXMLString, std::string());
        ~FakeParameter(){;} 

    protected:
        virtual Mantid::API::ImplicitFunctionParameter* cloneImp() const
        {
            return new FakeParameter;
        }
    };	



    class FakeImplicitFunction : Mantid::API::ImplicitFunction
    {
    public:
        bool evaluate(const Mantid::API::Point3D* pPoint3D) const
        {    
            return false;
        }
		MOCK_CONST_METHOD0(getName, std::string());
		MOCK_CONST_METHOD0(toXMLString, std::string());
    };

    class FakeFunctionBuilder : public Mantid::API::ImplicitFunctionBuilder
    {
    public:
        mutable bool isInvoked;
        void addParameter(std::auto_ptr<Mantid::API::ImplicitFunctionParameter> parameter)
        {
        }
        std::auto_ptr<Mantid::API::ImplicitFunction> create() const
        {
            isInvoked = true;
            return std::auto_ptr<Mantid::API::ImplicitFunction>(new FakeImplicitFunction);
        }
    };

public:

    void testCreate()
    {

        using namespace Mantid::MDAlgorithms;

        FakeFunctionBuilder* builderA = new FakeFunctionBuilder;
        FakeFunctionBuilder* builderB = new FakeFunctionBuilder;
        CompositeFunctionBuilder* innerCompBuilder = new CompositeFunctionBuilder;
        innerCompBuilder->addFunctionBuilder(builderA);
        innerCompBuilder->addFunctionBuilder(builderB);
        std::auto_ptr<CompositeFunctionBuilder> outterCompBuilder = std::auto_ptr<CompositeFunctionBuilder>(new CompositeFunctionBuilder);
        outterCompBuilder->addFunctionBuilder(innerCompBuilder);
        std::auto_ptr<Mantid::API::ImplicitFunction> topFunc = outterCompBuilder->create();
        //CompositeImplicitFunction* topCompFunc = dynamic_cast<CompositeImplicitFunction*>(topFunc.get());

        TSM_ASSERT("Nested builder not called by composite", builderA->isInvoked);
        TSM_ASSERT("Nested builder not called by composite", builderB->isInvoked);
        //TSM_ASSERT("Top level function generated, should have been a composite function instance.", topCompFunc != NULL);

    }

};



#endif