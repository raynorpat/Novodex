#ifndef NX_FOUNDATION_PROFILER
#define NX_FOUNDATION_PROFILER
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

#include "NxProfiler.h"
#define MAX_PROFILING_ZONES 64
#define MAX_PROFILING_STACK_DEPTH 256

const int NUM_PROFILE_TRACKER_HISTORY_SLOTS = 3;
const int NUM_THROWAWAY_UPDATES = 10;

#error unused?

class NXF_DLL_EXPORT Profiler : public NxProfiler 
	{
	struct History_Scalar 
		{
		double values[NUM_PROFILE_TRACKER_HISTORY_SLOTS];
		double variances[NUM_PROFILE_TRACKER_HISTORY_SLOTS];
		
		void update(double new_value, double *k_array);
		void eternity_set(double value);
		
		void clear();
		};
	
	struct Profile_Tracker_Data_Record 
		{
		int index;
		History_Scalar self_time;
		History_Scalar hierarchical_time;
		History_Scalar entry_count;
		double displayed_quantity;
		};

	public:
	Profiler();
	~Profiler();

    virtual void setMode(DisplayMode);

    virtual void update();

	//group access:

    static int stackData[MAX_PROFILING_STACK_DEPTH];
    static DefineZone *pointersData[MAX_PROFILING_ZONES];


	private:
	static int sort_records(const void *a, const void *b);
	double get_stdev(History_Scalar *scalar, int slot);

	//meta data:
    static History_Scalar frame_time;
    static History_Scalar integer_timestamps_per_second;

    static Profile_Tracker_Data_Record data_records[MAX_PROFILING_ZONES];
    static Profile_Tracker_Data_Record *sorted_pointers[MAX_PROFILING_ZONES];
    static double times_to_reach_90_percent[NUM_PROFILE_TRACKER_HISTORY_SLOTS];
    static double precomputed_factors[NUM_PROFILE_TRACKER_HISTORY_SLOTS];

    static int num_active_zones;
    static NxI64 last_integer_timestamp;
    static NxI64 current_integer_timestamp;
    static int update_index;
    static double last_update_time;
    static double dt;
    static double dt_per_integer_timestamp;
    static DisplayMode displayed_quantity;
	};
#endif