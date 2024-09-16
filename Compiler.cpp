// Fill out your copyright notice in the Description page of Project Settings.


#include "Compiler.h"
#include "Kismet/KismetMathLibrary.h"
#include "Serialization/ArchiveScriptReferenceCollector.h"

class FScriptBytecodeWriter : public FArchiveUObject
{
public:
	TArray<uint8>& ScriptBuffer;
public:
	FScriptBytecodeWriter(TArray<uint8>& InScriptBuffer)
		: ScriptBuffer(InScriptBuffer)
	{
	}

	void Serialize(void* V, int64 Length) override
	{
		int32 iStart = ScriptBuffer.AddUninitialized(IntCastChecked<int32, int64>(Length));
		FMemory::Memcpy(&(ScriptBuffer[iStart]), V, Length);
	}

	using FArchiveUObject::operator<<; // For visibility of the overloads we don't override

	FArchive& operator<<(FName& Name) override
	{
		FArchive& Ar = *this;

		// This must match the format and endianness expected by XFERNAME 
		FNameEntryId ComparisonIndex = Name.GetComparisonIndex(), DisplayIndex = Name.GetDisplayIndex();
		uint32 Number = Name.GetNumber();
		Ar << ComparisonIndex;
		Ar << DisplayIndex;
		Ar << Number;

		return Ar;
	}

	FArchive& operator<<(UObject*& Res) override
	{
		ScriptPointerType D = (ScriptPointerType)Res;
		FArchive& Ar = *this;

		Ar << D;
		return Ar;
	}

	FArchive& operator<<(FField*& Res) override
	{
		ScriptPointerType D = (ScriptPointerType)Res;
		FArchive& Ar = *this;
		Ar << D;
		return Ar;
	}

	FArchive& operator<<(FLazyObjectPtr& LazyObjectPtr) override
	{
		return FArchive::operator<<(LazyObjectPtr);
	}

	FArchive& operator<<(FSoftObjectPtr& Value) override
	{
		return FArchive::operator<<(Value);
	}

	FArchive& operator<<(FSoftObjectPath& Value) override
	{
		return FArchiveUObject::operator<<(Value);
	}

	FArchive& operator<<(TCHAR* S)
	{
		Serialize(S, FCString::Strlen(S) + 1);
		return *this;
	}

	FArchive& operator<<(EExprToken E)
	{
		checkSlow(E < 0xFF);

		uint8 B = static_cast<uint8>(E);
		Serialize(&B, 1);
		return *this;
	}

	FArchive& operator<<(ECastToken E)
	{
		uint8 B = static_cast<uint8>(E);
		Serialize(&B, 1);
		return *this;
	}

	FArchive& operator<<(EBlueprintTextLiteralType E)
	{
		static_assert(sizeof(__underlying_type(EBlueprintTextLiteralType)) == sizeof(uint8), "EBlueprintTextLiteralType is expected to be a uint8");

		uint8 B = (uint8)E;
		Serialize(&B, 1);
		return *this;
	}

	FArchive& operator<<(EPropertyType E)
	{
		uint8 B = static_cast<uint8>(E);
		Serialize(&B, 1);
		return *this;
	}

	CodeSkipSizeType EmitPlaceholderSkip()
	{
		CodeSkipSizeType Result = ScriptBuffer.Num();

		CodeSkipSizeType Placeholder = -1;
		(*this) << Placeholder;

		return Result;
	}

	void CommitSkip(CodeSkipSizeType WriteOffset, CodeSkipSizeType NewValue)
	{
		//@TODO: Any endian issues?
#if SCRIPT_LIMIT_BYTECODE_TO_64KB
		static_assert(sizeof(CodeSkipSizeType) == 2, "Update this code as size changed.");
		ScriptBuffer[WriteOffset] = NewValue & 0xFF;
		ScriptBuffer[WriteOffset + 1] = (NewValue >> 8) & 0xFF;
#else
		static_assert(sizeof(CodeSkipSizeType) == 4, "Update this code as size changed.");
		ScriptBuffer[WriteOffset] = NewValue & 0xFF;
		ScriptBuffer[WriteOffset + 1] = (NewValue >> 8) & 0xFF;
		ScriptBuffer[WriteOffset + 2] = (NewValue >> 16) & 0xFF;
		ScriptBuffer[WriteOffset + 3] = (NewValue >> 24) & 0xFF;
#endif
	}
};

// Sets default values
ACompiler::ACompiler()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACompiler::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACompiler::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

//A Test to Create a class that has one function for adding 2 local int variables and setting it to an instance variable
void ACompiler::Compile()
{	
	//UClass* NewClass = NewObject<UClass>(StaticClass()->GetPackage(), FName(TEXT("Test")), RF_Standalone);
	//GetTransientPackage() is also an option
	EPropertyFlags propFlags = CPF_Edit | CPF_BlueprintVisible | CPF_ZeroConstructor | CPF_IsPlainOldData | CPF_NoDestructor | CPF_HasGetValueTypeHash;
	//EPropertyFlags parmFlags = CPF_Parm | CPF_ZeroConstructor | CPF_IsPlainOldData;
	EPropertyFlags parmFlags = CPF_Parm | propFlags;

	UClass* NewClass = NewObject<UClass>(StaticClass()->GetPackage(), FName(TEXT("Test")), RF_Transient);	
	UFunction* Function = NewObject<UFunction>(UFunction::StaticClass(), FName(TEXT("MyTest")), RF_Public);
	NewClass->AddFunctionToFunctionMap(Function, FName(TEXT("MyTest")));

	FIntProperty* myVar = new FIntProperty(NewClass, FName(TEXT("MyVar")), RF_Public);
	myVar->SetPropertyFlags(propFlags);
	NewClass->AddCppProperty(myVar);
	
	NewClass->SetSuperStruct(UObject::StaticClass());

	FIntProperty* var1 = new FIntProperty(NewClass, FName(TEXT("Var1")), RF_NoFlags);
	FIntProperty* var2 = new FIntProperty(NewClass, FName(TEXT("Var2")), RF_NoFlags);
	FIntProperty* retVar = new FIntProperty(NewClass, FName(TEXT("RetVar")), RF_NoFlags);
	
	var1->SetPropertyFlags(propFlags);
	var2->SetPropertyFlags(propFlags);
	retVar->SetPropertyFlags(propFlags);	

	Function->AddCppProperty(retVar);
	Function->AddCppProperty(var2);
	Function->AddCppProperty(var1);
	Function->PropertiesSize = sizeof(int32) * 3;
	Function->FirstPropertyToInit = var1;
	Function->MinAlignment = 4;
	Function->PropertyLink = var1;
	Function->PropertyLink->Next = var2;
	Function->PropertyLink->Next->Next = retVar;
	Function->PostConstructLink = var1;
	Function->PostConstructLink->Next = var2;
	Function->PostConstructLink->Next->Next = retVar;
	//Function->ScriptAndPropertyObjectReferences = retVar;

	//Function->Bind();
	//Function->StaticLink();

	UFunction* Add = UKismetMathLibrary::StaticClass()->FindFunctionByName(TEXT("Add_IntInt"));

	TArray<uint8>& buffer = Function->Script;
	FScriptBytecodeWriter Writer(buffer);	

	int32 num = 5;

	Writer << EX_Let;
	//Writer << EExprToken::EX_InstanceVariable;
	Writer << EX_LocalVariable;
	Writer << var1;
	Writer << EX_IntConst;		
	Writer << num;

	Writer << EX_Let;
	//Writer << EExprToken::EX_InstanceVariable;
	Writer << EX_LocalVariable;
	Writer << var2;
	Writer << EX_IntConst;
	Writer << num;

	Writer << EX_Let;
	Writer << EX_LocalVariable;
	Writer << retVar;
	Writer << EX_CallMath;
	Writer << Add;
	Writer << EX_LocalVariable;
	Writer << var1;
	Writer << EX_LocalVariable;
	Writer << var2;
	Writer << EX_EndFunctionParms;

	Writer << EX_Let;
	Writer << EX_InstanceVariable;
	Writer << myVar;
	Writer << EX_LocalVariable;
	Writer << retVar;

	Writer << EX_Return;
	Writer << EX_Nothing;
	Writer << EX_EndOfScript;

	Function->Bind();
	Function->StaticLink();

	NewClass->Bind();
	NewClass->StaticLink();

	UObject* CDO = NewClass->GetDefaultObject();
	
	//FArchiveScriptReferenceCollector ObjRefCollector(Function->ScriptAndPropertyObjectReferences);
	//for (int32 iCode = 0; iCode < Function->Script.Num();)
	//{
	//	Function->SerializeExpr(iCode, ObjRefCollector);
	//}

	TArray<uint8> Args;
	//Args.Append((uint8*)&retVar, sizeof(int32));
	//Args.Append((uint8*)&var1, sizeof(int32));
	//Args.Append((uint8*)&var2, sizeof(int32));

	CDO->ProcessEvent(Function, NULL);


	//Writer.CloseScript();
}
