#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Compiler.generated.h"

UCLASS()
class PCGHYPE_API ACompiler : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACompiler();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	static void Compile();

};
