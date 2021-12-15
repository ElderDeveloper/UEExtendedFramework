﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "UEExtendedFloatTimeline.h"

UUEExtendedFloatTimeline* UUEExtendedFloatTimeline::ExtendedFloatTimeline(const FName TimelineName , UCurveFloat* FloatCurve ,const UObject* WorldContextObject , float PassTime , TEnumAsByte<EExtendedTimelinePlayType> TimelinePlayType)
{
	if (!ensure(WorldContextObject) && !ensure(FloatCurve))	return nullptr;

	if (!FloatCurve)
	{
		UE_LOG(LogBlueprint,Warning,TEXT("Float Curve Is Not Valid")); return nullptr;
	}
	if (FloatCurve->FloatCurve.IsEmpty())
	{
		UE_LOG(LogBlueprint,Warning,TEXT("Float Curve Is Valid But Its Empty")); return nullptr;
	}


	// Basic Reference To Timeline
	UUEExtendedFloatTimeline* Timeline;
	bool Exist = false;

	// Store The Current Timeline Time If We Use Play Or Reverse
	float TimelineCurrentTime = 0.f;

	// Check If We Have Already A Timeline With The Same Name
	if (const auto Action = ExtendedFloatTimelines.Find(TimelineName) )
	{
		// If We Have One Already Get Its Current Time And Destroy It So That Early Loops Stops
		Timeline =  Action->Get();
		
		if (Timeline)
		{
			TimelineCurrentTime = Timeline->TimePassed;
			Timeline->InternalCompleted();
		}
		
		Timeline = NewObject<UUEExtendedFloatTimeline>();
		Exist = true;

		// If We Did Not Create A New Timeline Finish This Function With Error
		if (!Timeline)	return nullptr;
		ExtendedFloatTimelines.Emplace(TimelineName , Timeline);
		
	}
	else // If We Dont Have One Just Create A New Timeline Object
	{
		Timeline = NewObject<UUEExtendedFloatTimeline>();
		Timeline->ExtendedTimelineName = TimelineName;
		Exist = false;

		// If We Did Not Create A New Timeline Finish This Function With Error
		if (!Timeline)	return nullptr;

		// Add Newly Created Timeline To Static Array So That We Can Check If It Exist Afterwards
		ExtendedFloatTimelines.Add(TimelineName,Timeline);
	}

	

	
	// Fill Simple Variables To Newly Created Timeline
	Timeline->WorldContext = WorldContextObject;
	Timeline->LoopTime = PassTime;
	Timeline->SelectedFloatCurve = FloatCurve;
	Timeline->CurveLastTime = FloatCurve->FloatCurve.GetLastKey().Time;
	Timeline->CurveFirstTime = FloatCurve->FloatCurve.GetFirstKey().Time;

	
	
	// Select What We Will Do For Newly Created Timeline
	switch (TimelinePlayType)
	{
		case Ext_Play:
			Timeline->TimelinePlaySide = Ext_Forward;
			Timeline->TimePassed = Exist ? TimelineCurrentTime : FloatCurve->FloatCurve.GetFirstKey().Time;
		return Timeline;

			
		case Ext_PlayFromStart:
			Timeline->TimelinePlaySide = Ext_Forward;
			Timeline->TimePassed = FloatCurve->FloatCurve.GetFirstKey().Time;
		return Timeline;
	
		case Ext_Reverse:
			Timeline->TimelinePlaySide = Ext_Backward;
			Timeline->TimePassed = Exist ? TimelineCurrentTime : FloatCurve->FloatCurve.GetLastKey().Time;
			return Timeline;
			
		case Ext_ReverseFromEnd:
			Timeline->TimelinePlaySide = Ext_Backward;
			Timeline->TimePassed = FloatCurve->FloatCurve.GetLastKey().Time;
		return Timeline;
			
		case Ext_StopTimeline:
			Timeline->InternalCompleted();
			return nullptr;
		
		default: return Timeline;
	}
}



void UUEExtendedFloatTimeline::ExtendedFloatTimelineManipulate(const FName TimelineName,const UObject* WorldContextObject, TEnumAsByte<EExtendedTimelinePlayType> TimelinePlayType)
{
	if (!ensure(WorldContextObject))	return;

	if (const auto Action = ExtendedFloatTimelines.Find(TimelineName) )
	{
		UUEExtendedFloatTimeline* Timeline =  Action->Get();
		if (!Timeline) return;
		// Select What We Will Do For Timeline
		switch (TimelinePlayType)
		{
			
			case Ext_Play:
				Timeline->TimelinePlaySide = Ext_Forward;
				return ;
				
			case Ext_PlayFromStart:
				Timeline->TimelinePlaySide = Ext_Forward;
				Timeline->TimePassed = Timeline->CurveFirstTime;
				return ;
				
			case Ext_Reverse:
				Timeline->TimelinePlaySide = Ext_Backward;
				return ;
				
			case Ext_ReverseFromEnd:
				Timeline->TimelinePlaySide = Ext_Backward;
				Timeline->TimePassed = Timeline->CurveLastTime;
				return ;
				
			case Ext_StopTimeline:
				Timeline->InternalCompleted();
				return;
		
			default: return;
		}
	}
}


void UUEExtendedFloatTimeline::InternalTick()
{
	if (SelectedFloatCurve)
	{
		Loop.Broadcast(SelectedFloatCurve->GetFloatValue(TimePassed));

		switch (TimelinePlaySide)
		{
		case Ext_Forward:
			
			TimePassed += LoopTime;
			if (TimePassed >= CurveLastTime)
			{
				Completed.Broadcast();
				InternalCompleted();
			}
			break;

		case Ext_Backward:

			TimePassed -= LoopTime;
			if (TimePassed <= CurveFirstTime)
			{
				Completed.Broadcast();
				InternalCompleted();
			}
			break;
		}
	}
}

void UUEExtendedFloatTimeline::InternalCompleted()
{
	if (WorldContext)
	{
		WorldContext->GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
		TimerHandle.Invalidate();
		SetReadyToDestroy();
		MarkPendingKill();
		if (const auto Action = ExtendedFloatTimelines.Find(ExtendedTimelineName) )
		{
			ExtendedFloatTimelines.Remove(ExtendedTimelineName);
		}
	}
}






void UUEExtendedFloatTimeline::Activate()
{
	Super::Activate();
	Reset();
}

void UUEExtendedFloatTimeline::Reset()
{
	if (TimerHandle.IsValid() && WorldContext)
		WorldContext->GetWorld()->GetTimerManager().ClearTimer(TimerHandle);

	if (WorldContext)
		WorldContext->GetWorld()->GetTimerManager().SetTimer(TimerHandle,this,&UUEExtendedFloatTimeline::InternalTick,LoopTime,true);
}

