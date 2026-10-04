/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#define NOMINMAX
#include "PerfDisplay.h"
#include "NxProfiler.h"
#include "ViewerPlatform.h"
#include "NxMath.h"
#include "ViewerPlatform.h"
#include <stdio.h>

NxProfiler::DefineZone perfDisplay("Perf display");

PerfDisplay::PerfDisplay()
	{
	}

PerfDisplay::~PerfDisplay()
	{
	}

void PerfDisplay::display()
	{
	const int IMPORTANT_SLOT = 1;
	const double SPEEDSTEP_DETECTION_RATIO = 0.08;
	
    //profile_tracker->draw(0, 300);
	
	NxProfiler::SetCurrentZone xx(perfDisplay);
	
    int slot = IMPORTANT_SLOT;
	
	
    // Draw the results.
	
	
	//position on screen:
    float sx = -0.95f;
	float col1 = -0.5f/2;
	float col2 = -0.2f/5;
    float sy = 0.7f;
	
	
    float hh = 0.06f; // font height
    // Draw the fps counter.
	
    float avg_frame_time = NxProfiler::frame_time.values[slot];
    if (avg_frame_time == 0) avg_frame_time = 0.01f;
    float fps = 1.0f / avg_frame_time;
	
    char *displayed_quantity_name = "*error*";
    switch (NxProfiler::displayed_quantity) {
		case NX_SELF_TIME:
			displayed_quantity_name = "SELF TIME";
			break;
		case NX_SELF_STDEV:
			displayed_quantity_name = "SELF STDEV";
			break;
		case NX_HIERARCHICAL_TIME:
			displayed_quantity_name = "HIERARCHICAL TIME";
			break;
		case NX_HIERARCHICAL_STDEV:
			displayed_quantity_name = "HIERARCHICAL STDEV";
			break;
		}
	
    static char buf[256];
    sprintf(buf, "fps: %3.2f  (Frame time %3.3fms)  Displaying %.100s", fps, avg_frame_time * 1000, displayed_quantity_name);
    
	static float colorPerf[4] = {1,0.7f,0.7f,1};
	ViewerUi::start();
	ViewerUi::draw(1, 32, sx, sy, buf, colorPerf);	//"Mode: Fly"
    sy -= 2*hh;
	
	// Detect SpeedStep and warn the user if it is screwing us up.
	//sy -= 1.5*hh;
	
	int ss_slot = 1;
	double ss_val = NxProfiler::integer_timestamps_per_second.values[ss_slot];
	double ss_variance = NxProfiler::integer_timestamps_per_second.variances[ss_slot] - ss_val*ss_val;
	double ss_stdev = NxMath::sqrt(NxMath::abs(ss_variance));
	float ss_ratio;
	if (ss_val) 
		ss_ratio = ss_stdev / fabs(ss_val);
	else 
		ss_ratio = 0;
		  
	if (ss_ratio > SPEEDSTEP_DETECTION_RATIO) 
		{
		char *warning1 = "WARNING: SpeedStep detected.  Results are unreliable!";
		char *warning2 = "(Try running on a desktop machine instead.)";
		ViewerUi::draw(1, 32, sx, sy, warning1);  sy -= hh;
		ViewerUi::draw(1, 32, sx, sy, warning2);  sy -= hh;
		}
	
	// Start drawing the actual report.
	float backup_sy = sy;
	int i;
	
	sy = backup_sy;
	
	// Draw the zone data.
	for (i = 0; i < NxProfiler::num_active_zones; i++) 
		{
		NxProfiler::Profile_Tracker_Data_Record *record = NxProfiler::sorted_pointers[i];
		float self_percentage = 100.0f * (record->self_time.values[slot] / NxProfiler::frame_time.values[slot]);
		float hier_percentage = 100.0f * (record->hierarchical_time.values[slot] / NxProfiler::frame_time.values[slot]);
		NX_ASSERT(hier_percentage > -1.0f);
		
		float self_ms = 1000 * record->self_time.values[slot];
		float hier_ms = 1000 * record->hierarchical_time.values[slot];
		
		NxProfiler::DefineZone *zone = NxProfiler::zone_pointers_by_index[record->index];
		
		//float name_width_used = 0.4f;//small_font->get_string_width_in_pixels(zone->name);
		//float name_offset = name_column_width - name_width_used - colon_width;
		
		float value = record->self_time.values[slot];
		float variance = record->self_time.variances[slot];
		variance = variance - value * value;
		if (variance < 0) variance = 0;
		float stdev = sqrt(variance);
	
		ViewerUi::draw(1, 32, sx, sy, (char *)zone->getName()/*, text_color.x, text_color.y, text_color.z*/);
		
		sprintf(buf, "%5.2f", record->displayed_quantity);
		//float num1_len = //small_font->get_string_width_in_pixels(buf);
		ViewerUi::draw(1, 32, col1, sy, buf/*, text_color.x, text_color.y, text_color.z*/);
		
		sprintf(buf, "%.2f", record->entry_count.values[1]);
		//float num2_len = small_font->get_string_width_in_pixels(buf);
		ViewerUi::draw(1, 32, col2, sy, buf/*, text_color.x, text_color.y, text_color.z*/);
		
		sy -= hh;
		}

	ViewerUi::end();

	}
