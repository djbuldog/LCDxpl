#include "datarefs.h"


void DataRef::assign(const std::string name, const std::string ser_code) {
	//std::cerr << "LCDxpl: Called assign with " << name << ", " << ser_code << std::endl;
	this->ser_code = ser_code;
	ptr = XPLMFindDataRef(name.c_str());
	changed = false;
}

const std::string DataRef::getSerStr() {
	this->changed = false;
	std::string retval(ser_code);
	return retval + " " + getValStr() + '\n';
}

/* ******************************************************************* */

void DataRefInt::update() {

	if (ptr == NULL) return;

	int val = XPLMGetDatai(this->ptr);
	if (val!=this->last) {
		changed = true;
		last=val;
	}  
}

const std::string DataRefInt::getValStr() {
	std::stringstream ss;
	ss << this->last;
  return ss.str();
}

/* ******************************************************************* */

const std::string DataRefIntInt::getValStr() {
	std::stringstream ss;
	int mhz, khz;
	
	mhz = this->last/100;
	khz = this->last%100;
	
	ss << mhz << "." << khz;
  return ss.str();
}

/* ******************************************************************* */

void DataRefFloat::update() {

	if (ptr == NULL) return;

	float val = XPLMGetDatai(this->ptr);
	if (val!=this->last) {
		changed = true;
		last=val;
	}  
}

const std::string DataRefFloat::getValStr() {
	std::stringstream ss;
	ss << this->last;
  return ss.str();
}

/* ******************************************************************* */

