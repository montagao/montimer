#include <iostream>
#include "GUI.h"

using namespace Graph_lib;

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

string tm_string(tm& current_t){
	string result;
	ostringstream s;
	s << std::setw(2) << setfill('0') << to_string(current_t.tm_hour);
	s << ":" << std::setw(2) << setfill('0') << to_string(current_t.tm_min);
	s << ":" << std::setw(2) << setfill('0') << to_string(current_t.tm_sec);
	return s.str();
}

string tm_string(int t){ // returns hh:mm:ss ouput in from seconds
	string result;
	ostringstream s;
	s << std::setw(2) << setfill('0') << (t/3600);
	t -= (t / 3600) * 3600;
	s << ":" << std::setw(2) << setfill('0') << (t / 60);
	t -= (t / 60) * 60;
	s << ":" << std::setw(2) << setfill('0') << t;
	return s.str();

}

struct Clock_hand : Shape {
	enum Type {
		second, minute, hour 
	};
	Clock_hand(Type t, Point p, int rr, struct tm* current_t)
		: hand{ t }, current_time(*current_t), r{rr},
		digital_time{ Point{ p.x - 80, p.y }, tm_string(current_time) }
	{
		if (hand == hour) {
			set_style(Line_style(Line_style::solid, 8));
		} else if (hand == minute) {
			set_style(Line_style(Line_style::solid, 5));
		}
		else if (hand == second) {
			set_color(Color::red);
		}

		digital_time.set_font_size(40);
		add(p);
		update_hand();
	}

	void displayDigital(bool b) 
	{
		dD = b;
	}


private: 
	Type hand;
	bool dD = false;
	int r;

	Text digital_time;
	Point endpoint;
	struct tm current_time;

	void draw_lines();

public:	

	tm& getTime()
	{
		return current_time;
	}

	int radius() const
	{
		return r;
	}

	void update_hand() // update hand to current time.
	{
		// update current time;
		update_time();
		double angle = find_angle();
		//std::cout << "Angle given: " << angle << "\n";

		int x_new, y_new;
		x_new = ((double)r * cos(angle)) + double(point(0).x);
		y_new = ((double)r * sin(angle)) + double(point(0).y);
		endpoint.x = x_new;
		endpoint.y = y_new;

	}

	void update_time() {
		time_t rawtime = time(0);
		localtime_s( &current_time, &rawtime);

		digital_time.set_label(tm_string(current_time));
	}

	double find_angle()
	{
		const double Pi = 3.1415926;
		double angle;
		double second_bias;
		double minute_bias;
		switch (hand)
		{ 
			// shift it by pi/2 since clock starts at 12 and not 3.
			// so if the we're at the 15 sec mark for instance we get the result
			// of sin( pi/2 - pi/2) result instead of sin( pi/2 );
			// also recall axis are flipped 

			case second:
				angle = ((2.0*Pi) / 60.0) * current_time.tm_sec  - Pi/2;
				return angle;
			case minute:
				// shift minute slightly depending on current second
				second_bias = ((2.0*Pi) / 60) * ((float)current_time.tm_sec / 60.0);
				angle = ((2.0 * Pi) / 60.0) * current_time.tm_min - Pi / 2 + second_bias;;
				return angle;
			case hour:
				// shift hour slightly depending on the current minute 
				minute_bias = ((2.0*Pi) / 12) * ((float)current_time.tm_min / 60.0);
				angle = ((2.0*Pi) / 12.0) * current_time.tm_hour - Pi/2 + minute_bias; 
				return angle;
			default : 
				return -1;
		}
	}

	void draw_lines() const
	{
		if (color().visibility() && !dD) // dD is bool of displayDigital
			fl_line(point(0).x, point(0).y, endpoint.x, endpoint.y);
		if (color().visibility() && dD == true)
			digital_time.draw_lines();
	}
};

struct Timer_window : Window
{
	/*
		creates second, minute, and hour hands
		refreshes current time and changes their pivots based on current time
		(doesn't increment on its own) - this way the clock is still accurate after the system goes to sleep or hangs.
	*/

	Timer_window(Point p, int x, int h, const string& title,string img_name) : Window{ p, x, h, title },
		bg{ Point{ clock_center.x - 300 / 2, clock_center.y - 300 / 2 }, img_name},
		clock_switch{ Point{ clock_center.x-50, clock_center.y + 160 }, 100, 30, "Analog/Digital", [](void*, void* pw) { static_cast<Timer_window*>(pw)->changeClock(); } },
		timer_presets{ Point{ 50, y_max() - 100 }, 90, 30, Menu::vertical, "presets" },
		timer_label{ Point{ clock_center.x - 50, clock_center.y + 220 }, "00:00:00" },
		pause_switch{ Point{ clock_center.x + 50, clock_center.y + 220 }, 100, 30, "Pause", [](void*, void* pw){ static_cast<Timer_window*>(pw)->pausePressed(); } },
		timer_in_h{ Point{ clock_center.x - 130, y_max() - 40 }, 60, 30, "" },
		timer_in_m{ Point{ clock_center.x - 60 , y_max() - 40 }, 60, 30, "" },
		timer_in_s{ Point{ clock_center.x + 10, y_max() - 40 }, 60, 30, "" },
		timer_switch{ Point{ clock_center.x + 50, clock_center.y + 260 }, 100, 30, "Start", [](void *, void* pw) { static_cast<Timer_window*>(pw)->timer_switchPressed();  } }
	{
		attach(clock_switch);
		attach(timer_switch);
		timer_presets.attach(new Button{ Point{ 0, 0 }, 150, 30, "10:00", [](void*, void* pw){static_cast<Timer_window*>(pw)->preset1(); } });

		attach(timer_in_h);
		attach(timer_in_m);
		attach(timer_in_s);

		attach(timer_presets);
		attach(timer_label);
		attach(pause_switch);


		time_t rawtime = time(0);
		struct tm* now = localtime(&rawtime);
		attach(bg);

		hands.push_back(new Clock_hand(Clock_hand::second, clock_center, 150, now));
		attach(hands[0]);

		hands.push_back(new Clock_hand(Clock_hand::minute,clock_center, 150, now));
		attach(hands[1]);

		hands.push_back(new Clock_hand(Clock_hand::hour, clock_center, 100, now));
		attach(hands[2]);

		Fl::add_timeout(0.001, Timer_cb, (void*)this);
	}
	// functions called by callback functions
	void update_hands() {
		//debug purposes	
		time_t rawtime = time(0);
		struct tm* current_time = localtime(&rawtime);
		std::cout << asctime(current_time);

		for (int i = 0; i < hands.size(); i++)
		{
			hands[i].update_time();
			if (!displayDigital) 
				hands[i].update_hand();
		}
		int now = (*current_time).tm_sec;
		if (secsRemaining && !timerPaused)
		{
			if (timer_label.color().as_int() != 0)
				timer_label.set_color(0);
			if (now != prevSec)
			{
				prevSec = now;
				secsRemaining--;
			}
			timer_label.set_label(tm_string(secsRemaining));
		}
		else if (timerPaused || !timerStarted)
		{
			if (now != prevSec )
			{ 
				if (timer_label.color().as_int() == 0) // Color::red corresponds to the 32 bit int const of FL_RED
				{
					timer_label.set_color(50);
				}
				else {
					timer_label.set_color(0);
				}
				prevSec = now;
			}
		}
		else
		{
			if (now != prevSec )
			{ 
				if (timer_label.color().as_int() == FL_RED) // Color::red corresponds to the 32 bit int const of FL_RED
				{
					timer_label.set_color(50);
				}
				else {
					timer_label.set_color(Color::red);
				}
				prevSec = now;
			}
		}
		redraw();
	}

	void changeClock(){
		displayDigital = !displayDigital;
		// when switching from digital to analog hide the clock img and hands.
		for (int i = 0; i < hands.size(); i++)
			hands[i].displayDigital(displayDigital);
		if (displayDigital) {
			bg.set_mask(Point{ 0, 0 }, 1, 1);
		}
		else {
			bg.set_color(Color::visible);
			bg.set_mask(Point{ 0, 0 }, 1000, 1000);
		}
	}

	void pausePressed()
	{
		// unpause if already paused
		if (timerPaused) {
			timerPaused = false;
			pause_switch.label = "Pause";
		}
		else {
			pause_switch.label = "Unpause";
			timerPaused = true;
		}
	}
	
	void timer_switchPressed()
	{
		if (timerStarted) {
			secsRemaining = 0;
			timer_label.set_label(tm_string(0));
			timer_switch.label = "Start";
			timerStarted = false;
		} else {
			timerStarted = true;
			timer_switch.label = "Stop";
			int t = timer_in_h.get_int() * 3600 + timer_in_m.get_int() * 60 + timer_in_s.get_int();
			startTimer(t);
		}
	}

	void preset1()
	{
		startTimer(600);
	}

	void startTimer(int t) // time to countdown (in seconds);
	{
		timerPaused = false;
		timerStarted = true;

		timer_switch.label = "Stop";
		secsRemaining = t;
		prevSec = hands[0].getTime().tm_sec;
	}
	
	static void Timer_cb(Address pw) {
		reference_to<Timer_window>(pw).update_hands();
		Fl::repeat_timeout(0.15, Timer_cb,pw); // decreasing timeout time increases accuracy at cost of performance

	}
private:

	Point clock_center{ 150, 150 };
	Image bg;

	bool displayDigital = false;
	bool timerPaused = true;
	bool timerStarted = false;
	Text timer_label;

	int secsRemaining = 1; // give  some time remaining by default so it doesnt flash
	int prevSec;

	Vector_ref<Clock_hand> hands;
	Button clock_switch;
	Button pause_switch;
	Button timer_switch;

	In_box timer_in_h;
	In_box timer_in_m;
	In_box timer_in_s;

	Menu timer_presets;


};

 
int main()
{
	try{

		Timer_window win{ Point{ 0, 0 }, 310, 500, "MontClock", "Clock_bg.gif" };

		return gui_main();
	}
	catch (exception & e)
	{
		cerr << "Exception : " << e.what() << '\n';
		return 1;
	}
	

}