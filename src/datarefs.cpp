#include "datarefs.h"
#include <cstring>
#include <iomanip>  // setprecision

void DataRef::assign(const std::string name, const std::string ser_code) {
	//std::cerr << "LCDxpl: Called assign with " << name << ", " << ser_code << std::endl;
	this->ser_code = ser_code;
	ptr = XPLMFindDataRef(name.c_str());
	changed = false;
}

const std::string DataRef::getSerStr() {
	changed = false;
	std::string retval(ser_code);
	return retval + getValStr() + '\n';
}

/* ******************************************************************* */

void DataRefInt::update(std::string command) {

	if (ptr == NULL) return;

	int val = XPLMGetDatai(ptr);
	if (val!=last) {
		changed = true;
		last=val;
	}

	if (!command.empty()) {
		if (strncmp(command.c_str(),ser_code.c_str(),ser_code.size()) == 0) {
			std::istringstream is(command.c_str()+ser_code.size());
			is >> val;
			if (val!=last) {
				XPLMSetDatai(ptr,val);
				changed = false;
				last=val;
			}
		}
	}

}

const std::string DataRefInt::getValStr() {
	std::stringstream ss;
	ss << last;
	return ss.str();
}

/* ******************************************************************* */

void DataRefIntInt::update(std::string command) {

	if (ptr == NULL) return;

	int val = XPLMGetDatai(ptr);
	if (val!=last) {
		changed = true;
		last=val;
	}

	if (!command.empty()) {
		if (strncmp(command.c_str(),ser_code.c_str(),ser_code.size()) == 0) {
			std::istringstream is(command.c_str()+ser_code.size());
			float val2;
			is >> val2;
			val = val2*100;
			if (val!=last) {
				XPLMSetDatai(ptr,val);
				changed = false;
				last=val;
			}
		}
	}

}

const std::string DataRefIntInt::getValStr() {
	std::stringstream ss;
	int mhz, khz;

	mhz = last/100;
	khz = last%100;

	ss << mhz << "." << khz;
	return ss.str();
}

/* ******************************************************************* */

void DataRefFloat::update(std::string command) {

	if (ptr == NULL) return;

	float val = XPLMGetDataf(ptr);
	if (val!=last) {
		changed = true;
		last=val;
	}

	if (!command.empty()) {
		if (strncmp(command.c_str(),ser_code.c_str(),ser_code.size()) == 0) {
			std::istringstream is(command.c_str()+ser_code.size());
			is >> val;
			if (val!=last) {
				XPLMSetDataf(ptr,val);
				changed = false;
				last=val;
			}
		}
	}

}

const std::string DataRefFloat::getValStr() {
	std::stringstream ss;
	ss << std::setprecision(1) << last;
	return ss.str();
}

/* ******************************************************************* */

void DataRefFloatInt::update(std::string command) {

	if (ptr == NULL) return;

	// 0.4 is for rounding the number
	int val = int((XPLMGetDataf(ptr)*prec)+0.4);
	if (val!=last) {
		changed = true;
		last=val;
	}

	if (!command.empty()) {
		if (strncmp(command.c_str(),ser_code.c_str(),ser_code.size()) == 0) {
			std::istringstream is(command.c_str()+ser_code.size());
			is >> val;
			if (val!=last) {
				XPLMSetDataf(ptr,float(val/prec));
				changed = false;
				last=val;
			}
		}
	}

}

const std::string DataRefFloatInt::getValStr() {
	std::stringstream ss;
	ss << last;
	return ss.str();
}

/* ******************************************************************* */

