#pragma once

// if you add an animation here, you must also add one in the animation_names
// array in animation.cpp and update the array length here accordingly
enum animation {
	ANIM_OFF,
	ANIM_NIGHT,
	ANIM_RAINBOW,
	ANIM_BOUNCE,
	ANIM_CYCLE,
	ANIM_STROBE,
	ANIM_WHITE,
	ANIM_UNDEFINED, // must be last
};


extern const char * const animation_names[8];
extern enum animation anim, new_anim;
extern double max_brightness;
extern double rainbow_density;
extern double cycle_speed;
extern double strobe_speed;

void wifi_ap(void);
void wifi_fail(void);
void begin_anim(void);
void cycle_anim(void);
