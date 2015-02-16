// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


typedef int32 NodeIdType;
typedef int32 ConnectionIdType;


struct FNodeData
{
	FVector Min;
	FVector Max;

//	TArray< struct FConnectionData* > Outgoing;
	TArray< ConnectionIdType > Outgoing;

	inline FVector Center() const
	{
		return (Min + Max) * 0.5f;
	}

	inline FBox Box() const
	{
		return FBox{ Min, Max };
	}

	inline double Volume() const
	{
		auto Extent = Max - Min;
		return (double)Extent.X * (double)Extent.Y * (double)Extent.Z;
	}
};

struct FConnectionData
{
//	const FNodeData* Src;
//	const FNodeData* Dest;
	NodeIdType Src;
	NodeIdType Dest;

	FBox Portal;
};




