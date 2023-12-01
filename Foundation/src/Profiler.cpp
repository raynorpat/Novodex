/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "Nxf.h"
#include "NxMath.h"
#include "NxProfiler.h"
#include "NxTime.h"
#include "FoundationSDK.h"
/**
This profiler is a modified version of the "low level" portion of the code
published with the article "Interactive Profiling" in the Dec 2002 GDMag.

  This program is Copyright (c) 2002 Jonathan Blow.  All rights reserved.
  Permission to use, modify, and distribute a modified version of 
  this software for any purpose and without fee is hereby granted, 
  provided that the above copyright notice appear in all copies and 
  that both the copyright notice and this permission notice appear 
  in supporting documentation and that the source code is provided 
  to the user.
  
	THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
	AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
	INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
	FITNESS FOR A PARTICULAR PURPOSE.
*/
/*
Customized by Novodex.
*/

//statics:
bool NxProfiler::initialized = false;
NxProfiler::DefineZone NxProfiler::defaultZone;
int NxProfiler::stackData[NX_MAX_PROFILING_STACK_DEPTH];
NxProfiler::DefineZone *NxProfiler::pointersData[NX_MAX_PROFILING_ZONES];

//NxProfiler * NxProfiler::instance = 0;
int NxProfiler::stackSize = NX_MAX_PROFILING_ZONES;
int NxProfiler::maxZones = NX_MAX_PROFILING_STACK_DEPTH;
int NxProfiler::current_zone = 0;
int NxProfiler::stack_pos = 0;
int NxProfiler::num_zones = 0;
int * NxProfiler::zone_stack = &NxProfiler::stackData[0];
NxProfiler::DefineZone **NxProfiler::zone_pointers_by_index = &NxProfiler::pointersData[0];


NxProfiler::History_Scalar NxProfiler::frame_time;
NxProfiler::History_Scalar NxProfiler::integer_timestamps_per_second;

NxProfiler::Profile_Tracker_Data_Record NxProfiler::data_records[NX_MAX_PROFILING_ZONES];
NxProfiler::Profile_Tracker_Data_Record *NxProfiler::sorted_pointers[NX_MAX_PROFILING_ZONES];
double NxProfiler::times_to_reach_90_percent[NX_NUM_PROFILE_TRACKER_HISTORY_SLOTS];
double NxProfiler::precomputed_factors[NX_NUM_PROFILE_TRACKER_HISTORY_SLOTS];

int NxProfiler::num_active_zones;
NxI64 NxProfiler::last_integer_timestamp;
NxI64 NxProfiler::current_integer_timestamp;
int NxProfiler::update_index;
double NxProfiler::last_update_time;
double NxProfiler::dt;
double NxProfiler::dt_per_integer_timestamp;
NxDisplayMode NxProfiler::displayed_quantity;

int *dest;
#define VARIANCE_TOLERANCE_FACTOR 0.5
#define FRAME_TIME_INITIAL 0.001
#define ITERATIONS 10000000
#define my_busy_double 1
#define IMPORTANT_SLOT 1
 



NxProfiler::DefineZone::DefineZone(const char *_name) 
	{
    name = _name;
    index = ++(NxProfiler::num_zones);	//this starts w. 1 because default zone is 0.
    NX_ASSERT(index < NX_MAX_PROFILING_ZONES);
    NxProfiler::zone_pointers_by_index[index] = this;
	
    total_self_ticks = 0;
    total_hier_ticks = 0;
    t_self_start = 0;
    t_hier_start = 0;
	
    total_entry_count = 0;
	}

NxProfiler::DefineZone::DefineZone() 
	{
	//this gets called for ctor of defaultZone.
	NxProfiler::initialize();
	}

NxProfiler::DefineZone::~DefineZone() 
	{
	//zones must now be dynamically deleteable!
	//remove this from the zone list
	//replace this with the last
	NxProfiler::zone_pointers_by_index[index] = NxProfiler::zone_pointers_by_index[NxProfiler::num_zones];
	NxProfiler::zone_pointers_by_index[index]->index = index;
	if (NxProfiler::num_zones > 0)
		NxProfiler::num_zones --;

	}


void NxProfiler::DefineZone::release()
	{
	delete this;
	}

void NxProfiler::DefineZone::enter()
	{
	NxProfiler::Enter_Zone(*this);
	}

void NxProfiler::DefineZone::leave()
	{
	NxProfiler::Exit_Zone();
	}


void NxProfiler::initialize() 
	{
	if (initialized)
		return;

	initialized = true;
	
	initializeHighLevel();
	
	
    defaultZone.name = "*uncharted*";
    defaultZone.index = 0;
    NX_ASSERT(defaultZone.index < NX_MAX_PROFILING_ZONES);
    NX_ASSERT(NxProfiler::zone_pointers_by_index[defaultZone.index] == 0);
    NxProfiler::zone_pointers_by_index[defaultZone.index] = &defaultZone;
	
    defaultZone.total_self_ticks = 0;
    defaultZone.total_hier_ticks = 0;
    defaultZone.t_self_start = 0;
    defaultZone.t_hier_start = 0;	
    defaultZone.total_entry_count = 0;
	
	
	
	
	
    DefineZone *zone = zone_pointers_by_index[0];
    NX_ASSERT(zone != 0);
    
    getTime(&zone->t_self_start);
    zone->t_hier_start = zone->t_self_start;
	}



///////////////////High Level Meta Stuff//////////////////////////

int NxProfiler::sort_records(const void *a, const void *b) 
	{
    Profile_Tracker_Data_Record *record_a = *(Profile_Tracker_Data_Record **)a;
    Profile_Tracker_Data_Record *record_b = *(Profile_Tracker_Data_Record **)b;
	
    double time_a = record_a->displayed_quantity;
    double time_b = record_b->displayed_quantity;
	
    if (time_a < time_b) return +1;
    if (time_a > time_b) return -1;
    return 0;
	}

void NxProfiler::initializeHighLevel() 
	{
    update_index = 0;
    last_update_time = 0;
	
    times_to_reach_90_percent[0] = 0.1;
    times_to_reach_90_percent[1] = 0.8;
    times_to_reach_90_percent[2] = 2.5;
	
    displayed_quantity = NX_SELF_TIME;
	
    int i;
    for (i = 0; i < NX_MAX_PROFILING_ZONES; i++) 
		{
        Profile_Tracker_Data_Record *record = &data_records[i];
        record->index = -1;
		
        record->self_time.clear();
        record->hierarchical_time.clear();
        record->entry_count.clear();
		}
	
    frame_time.clear();
	
    int j;
    for (j = 0; j < NX_NUM_PROFILE_TRACKER_HISTORY_SLOTS; j++) 
		{
        frame_time.values[j] = FRAME_TIME_INITIAL;
		}
	}
/*
NxProfiler::~NxProfiler()
	{
	//nothing?
	}
*/

void NxProfiler::setMode(NxDisplayMode desired) 
	{
    displayed_quantity = desired;
	}


double NxProfiler::get_stdev(History_Scalar *scalar, int slot) 
	{
    double value = scalar->values[slot];
    double variance = scalar->variances[slot];
    variance = variance - value * value;
    if (variance < 0) variance = 0;
    double stdev = NxMath::sqrt(variance);
    return stdev;
	}

void NxProfiler::update() 
	{
	static NxFoundation::Time myTime;
    // Precompute the time factors
	
/*	
    if (update_index == 0) {
        dt = FRAME_TIME_INITIAL;
		} else {
        dt = now - last_update_time;
			}
*/
	    dt = myTime.GetElapsedSeconds();

		
		//    if (dt < FRAME_TIME_INITIAL) dt = FRAME_TIME_INITIAL;
		
		//last_update_time = now;
		
		
		int i;
		for (i = 0; i < NX_NUM_PROFILE_TRACKER_HISTORY_SLOTS; i++) 
			{
			precomputed_factors[i] = NxMath::pow((NxF64)0.1, (NxF64)dt / times_to_reach_90_percent[i]);
			}
		
		precomputed_factors[0] = 0; // For now we want this to be instantaneous.
		
		NxI64 timestamp_delta;
		getTime(&current_integer_timestamp);
		if (update_index == 0) 
			{
			NxI64 sum = 0;
			for (i = 0; i < num_active_zones; i++) 
				{
				sum += zone_pointers_by_index[i]->total_self_ticks;
				}
			
			if (sum == 0) 
				sum = 1;
			
			timestamp_delta = sum;
			} 
		else 
			{
			timestamp_delta = current_integer_timestamp - last_integer_timestamp;
			if (timestamp_delta == 0) timestamp_delta = 1;
			}
		
		last_integer_timestamp = current_integer_timestamp;
		
		double timestamps_per_second = (double)timestamp_delta / dt;
		if (update_index < NX_NUM_THROWAWAY_UPDATES) 
			{
			integer_timestamps_per_second.eternity_set(timestamps_per_second);
			} 
		else 
			{
			integer_timestamps_per_second.update(timestamps_per_second, precomputed_factors);
			}
		
				double timestamps_to_seconds;
				if (timestamps_per_second) 
					{
					timestamps_to_seconds = 1.0 / timestamps_per_second;
					} 
				else 
					{
					timestamps_to_seconds = 0;
					}
				
				for (i = 0; i < NX_MAX_PROFILING_ZONES; i++) 
					{
					DefineZone *zone = zone_pointers_by_index[i];
					if (zone == NULL) break;
					
					Profile_Tracker_Data_Record *record = &data_records[i];
					
					record->index = zone->index;
					
					double self_time = zone->total_self_ticks * timestamps_to_seconds;
					double hier_time = zone->total_hier_ticks * timestamps_to_seconds;
					
					double entry_count = zone->total_entry_count;
					
					NX_ASSERT(hier_time >= 0);
					
					if (update_index < NX_NUM_THROWAWAY_UPDATES) 
						{
						int j;
						for (j = 0; j < NX_NUM_PROFILE_TRACKER_HISTORY_SLOTS; j++) 
							{
							record->self_time.values[j] = self_time;
							record->self_time.variances[j] = self_time;
							record->hierarchical_time.values[j] = hier_time;
							record->hierarchical_time.variances[j] = hier_time;
							record->entry_count.values[j] = entry_count;
							record->entry_count.variances[j] = entry_count;
							}
						} 
					else 
						{
						// @Improvement:  If we want to be very special, we can verify here that
						// our self_time and hier_times roughly add up to dt.  If they don't, then
						// maybe SpeedStep kicked in, or something else happened to disrupt the
						// accuracy of our integer timestamps, and they should not be trusted
						// this frame...
						record->self_time.update(self_time, precomputed_factors);
						record->hierarchical_time.update(hier_time, precomputed_factors);
						record->entry_count.update(entry_count, precomputed_factors);
						}
					
					// XXX This should probably be cleared out only by a core system thing
					// that runs once per frame (i.e. we should be able to instantiate
					// multiple profilers looking at different things and they should all
					// work)
					zone->total_self_ticks = 0;
					zone->total_hier_ticks = 0;
					zone->total_entry_count = 0;
					}
				
				frame_time.update(dt, precomputed_factors);
				
				update_index++;
				
				
				int slot = IMPORTANT_SLOT;
				
				int cursor = 0;
				for (i = 0; i < NX_MAX_PROFILING_ZONES; i++) 
					{
					Profile_Tracker_Data_Record *record = &data_records[i];
					if (record->index == -1) continue;
					
					switch (displayed_quantity) 
						{
						case NX_SELF_TIME:
							record->displayed_quantity = record->self_time.values[slot] * 1000.0;
							break;
						case NX_SELF_STDEV: 
							{
							double stdev = get_stdev(&record->self_time, slot);
							record->displayed_quantity = stdev * 1000.0;
							break;
							}
						case NX_HIERARCHICAL_TIME:
							record->displayed_quantity = record->hierarchical_time.values[slot] * 1000.0;
							break;
						case NX_HIERARCHICAL_STDEV: 
							{
							double stdev = get_stdev(&record->hierarchical_time, slot);
							record->displayed_quantity = stdev * 1000.0;
							break;
							}
						default:
							NX_ASSERT(0);
						}
					
					sorted_pointers[cursor++] = record;
					}
				
				num_active_zones = cursor;
				
				qsort(sorted_pointers, num_active_zones, sizeof(Profile_Tracker_Data_Record *), sort_records);
				
				double total_self_time[NX_NUM_PROFILE_TRACKER_HISTORY_SLOTS];
				
				int j;
				for (j = 0; j < NX_NUM_PROFILE_TRACKER_HISTORY_SLOTS; j++) 
					{
					double sum = 0;
					for (i = 0; i < num_active_zones; i++) 
						{
						sum += sorted_pointers[i]->self_time.values[j];
						}
					
					if (sum == 0) sum = 0.01;
					total_self_time[j] = sum;
					}
}

void NxProfiler::History_Scalar::clear() 
	{
    int i;
    for (i = 0; i < NX_NUM_PROFILE_TRACKER_HISTORY_SLOTS; i++) 
		{
        values[i] = 0;
        variances[i] = 0;
		}
	}

void NxProfiler::History_Scalar::update(double new_value, double *k_array) 
	{
    
    double new_variance = new_value * new_value;
	
    int i;
    for (i = 0; i < NX_NUM_PROFILE_TRACKER_HISTORY_SLOTS; i++) 
		{
        double k = k_array[i];
        values[i] = values[i] * k + new_value * (1 - k);
        variances[i] = variances[i] * k + new_variance * (1 - k);
		}
	}

void NxProfiler::History_Scalar::eternity_set(double new_value) 
	{
    double new_variance = new_value * new_value;
	
    int i;
    for (i = 0; i < NX_NUM_PROFILE_TRACKER_HISTORY_SLOTS; i++) 
		{
        values[i] = new_value;
        variances[i] = new_variance;
		}
	}



///////////The below used to be inlines, but that confuses the linker when making the class be exported:

inline void NxProfiler::getTime(NxI64 *result) 
	{
    int *dest = (int *)(result);
#ifdef WIN32
    __asm {
        rdtsc;
        mov    ebx, dest;
        mov    [ebx], eax;
        mov    [ebx+4], edx;
	}
#elif LINUX
    asm( "rdtsc\n\t"
    	"mov %eax, dest\n\t"
    	"mov %edx, dest+4\n\t"
	);
#else
#error Unknown platform!
#endif
	}
/*
inline NxProfiler * NxProfiler::getInstance()
	{
	return //instance;	//there is a global instance created in Foundation.
	}
*/

void NxProfiler::Enter_Zone(DefineZone &zone) 
	{
    DefineZone *old_zone = zone_pointers_by_index[current_zone];
    zone_stack[stack_pos] = current_zone;
    current_zone = zone.index;
	
    getTime(&zone.t_self_start);
    zone.t_hier_start = zone.t_self_start;
	
    old_zone->total_self_ticks += zone.t_self_start - old_zone->t_self_start;
    zone.total_entry_count++;
	
    stack_pos++;
	}

void NxProfiler::Exit_Zone() 
	{
    DefineZone *zone = zone_pointers_by_index[current_zone];
	
    --stack_pos;
	
    NxI64 t_end;
    getTime(&t_end);
	
    zone->total_self_ticks += (t_end - zone->t_self_start);
    zone->total_hier_ticks += (t_end - zone->t_hier_start);
	
    current_zone = zone_stack[stack_pos];
	
    DefineZone *new_zone = zone_pointers_by_index[current_zone];
    new_zone->t_self_start = t_end;
	}



NxProfiler::SetCurrentZone::SetCurrentZone(DefineZone &zone) 
	{
    NxProfiler::Enter_Zone(zone);
	}

NxProfiler::SetCurrentZone::~SetCurrentZone() 
	{
    NxProfiler::Exit_Zone();
	}

const char * NxProfiler::DefineZone::getName() 
	{ 
	return name; 
	}
