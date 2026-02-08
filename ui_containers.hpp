#pragma once

#include <cstdint>

struct mouse_probe {
	double x;
	double y;
	uint64_t control_id = 0;
        uint64_t last_frame_control_id = 0;
	uint64_t window_id = 0;
	uint64_t last_frame_window_id = 0;
};
