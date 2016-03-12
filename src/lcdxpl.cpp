#include <cstring>
#include <string>
#include <iostream>
#include <vector>
#include "XPLMProcessing.h"
#include "XPLMMenus.h"

#include "serial.h"
#include "datarefs.h"
#include "commands.h"

/* Global variables */
static std::vector<DataRef*> datarefs;
static std::vector<Command*> commands;
static Serial ser;
static float callback_rate = -2.0;

float FlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);
void LCDxplMenuHandler(void *, void *);

#define DEFDEVICE "/dev/ttyACM1"
struct menu {
	XPLMMenuID	id;
	std::vector<std::string> devlist;
	long selected;
} mymenu;

void createMenu(std::string def) {

	mymenu.devlist = ser.getDevList();
	mymenu.devlist.insert(mymenu.devlist.begin(),"none");
	mymenu.selected = 0;

	for(std::vector<std::string>::iterator it = mymenu.devlist.begin(); it != mymenu.devlist.end(); ++it) {
		std::cerr << "comparing '" << *it << "' to '" << DEFDEVICE << "'" << std::endl;
		if ((*it) == def) mymenu.selected = it - mymenu.devlist.begin();
	}

	long id = 0;
	for(std::vector<std::string>::iterator it = mymenu.devlist.begin(); it != mymenu.devlist.end(); ++it) {
		std::cerr << "adding '" << it->c_str() << "' with id " << id << std::endl;
		XPLMAppendMenuItem(mymenu.id, it->c_str(), (void *) id, 1);
		XPLMCheckMenuItem(mymenu.id, id, (id==mymenu.selected)?xplm_Menu_Checked:xplm_Menu_Unchecked);
		++id;
	}

	XPLMAppendMenuSeparator(mymenu.id);
	XPLMAppendMenuItem(mymenu.id, "refresh", (void *) 99, 1);

}

PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc) {

	int PluginSubMenuItem;

	PluginSubMenuItem = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "LCDxpl port", NULL, 1);
	mymenu.id = XPLMCreateMenu("LCDxpl port", XPLMFindPluginsMenu(), PluginSubMenuItem, LCDxplMenuHandler, NULL);

	createMenu(DEFDEVICE);
	ser.open(mymenu.devlist.at(mymenu.selected));

	strcpy(outName, "LCDxpl");
	strcpy(outSig, "bari2.eu");
	strcpy(outDesc, "A plugin for X-Plane LCD.");

	commands.push_back(new Command("sim/GPS/g430n1_page_up","G1p"));
	commands.push_back(new Command("sim/GPS/g430n1_page_dn","G1l"));
	commands.push_back(new Command("sim/GPS/g430n1_chapter_up","G2p"));
	commands.push_back(new Command("sim/GPS/g430n1_chapter_dn","G2l"));

	commands.push_back(new Command("sim/GPS/g430n1_cursor","Gb2"));
	commands.push_back(new Command("sim/GPS/g430n1_ent","Gb1"));

	datarefs.push_back(new DataRefInt("sim/cockpit2/radios/actuators/transponder_code","SQK"));
	datarefs.push_back(new DataRefInt("sim/cockpit2/radios/actuators/adf1_frequency_hz","ADF"));

	datarefs.push_back(new DataRefFloatInt("sim/cockpit2/radios/indicators/gps_dme_distance_nm", "DIS", 10));
	datarefs.push_back(new DataRefFloatInt("sim/cockpit/radios/gps_dme_speed_kts","GSp",10));
	datarefs.push_back(new DataRefFloatInt("sim/cockpit/radios/gps_dme_time_secs","ETE",60));
	datarefs.push_back(new DataRefFloatInt("sim/cockpit/radios/gps_course_degtm","DTK"));
	datarefs.push_back(new DataRefFloatInt("sim/cockpit2/radios/indicators/gps_bearing_deg_mag","DBG"));
	datarefs.push_back(new DataRefFloatInt("sim/flightmodel/position/magpsi","TRK"));  

	datarefs.push_back(new DataRefIntInt("sim/cockpit2/radios/actuators/com1_frequency_hz","C1u"));
	datarefs.push_back(new DataRefIntInt("sim/cockpit2/radios/actuators/com1_standby_frequency_hz","C1s"));
	datarefs.push_back(new DataRefIntInt("sim/cockpit2/radios/actuators/nav1_frequency_hz","N1u"));
	datarefs.push_back(new DataRefIntInt("sim/cockpit2/radios/actuators/nav1_standby_frequency_hz","N1s"));

	for(std::vector<DataRef*>::iterator it = datarefs.begin(); it != datarefs.end(); ++it) {
		if (!(*it)->isInit()) {
			std::cerr << "LCDxpl: Some dataref obj is not initialized!!" << std::endl;
			return 0;
		}
	}

	for(std::vector<Command*>::iterator it = commands.begin(); it != commands.end(); ++it) {
		if (!(*it)->isInit()) {
			std::cerr << "LCDxpl: Some command obj is not initialized!!" << std::endl;
			return 0;
		}
	}

	return 1;
}

PLUGIN_API void	XPluginStop(void) {

	for(std::vector<DataRef*>::iterator it = datarefs.begin(); it != datarefs.end(); ++it) {
		delete *it;
	}
	datarefs.clear();

	ser.close();

}

PLUGIN_API void XPluginDisable(void) {

	XPLMUnregisterFlightLoopCallback(FlightLoopCallback, NULL);

}

PLUGIN_API int XPluginEnable(void) {

	XPLMRegisterFlightLoopCallback(FlightLoopCallback,callback_rate,NULL);
	return 1;

}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void *inParam) {
}

float FlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon) {

	std::string command(ser.readln());
	if (!command.empty()) {
		std::cerr << "rec: " << command << std::endl;
	}

	for(std::vector<Command*>::iterator it = commands.begin(); it != commands.end(); ++it) {
		(*it)->perform(command);
	}

	for(std::vector<DataRef*>::iterator it = datarefs.begin(); it != datarefs.end(); ++it) {
		(*it)->update(command);
		if ((*it)->isChanged()) {
			ser.write((*it)->getSerStr());
		}
	}

	return callback_rate;

}

void LCDxplMenuHandler(void * inMenuRef, void * inItemRef) {

	if (((long)inItemRef != 99)&&((long)inItemRef != mymenu.selected)) {
		XPLMCheckMenuItem(mymenu.id, mymenu.selected, xplm_Menu_Unchecked);
		mymenu.selected = (long)inItemRef;
		XPLMCheckMenuItem(mymenu.id, mymenu.selected, xplm_Menu_Checked);
		std::cerr << "switching to " << mymenu.devlist.at(mymenu.selected) << std::endl;
		ser.close();
		ser.open(mymenu.devlist.at(mymenu.selected));
	}

	if ((long)inItemRef == 99) {
		XPLMClearAllMenuItems(mymenu.id);
		createMenu(mymenu.devlist.at(mymenu.selected));
		if (!mymenu.selected) ser.close();
	}

}

