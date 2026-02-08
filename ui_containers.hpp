#pragma once

#include <cstdint>

struct mouse_probe {
	double x;
	double y;
	int64_t control_id = -1;
        int64_t last_frame_control_id = -1;
	int64_t window_id = -1;
	int64_t last_frame_window_id = -1;
};
