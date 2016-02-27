#include <sstream>
#include "XPLMDataAccess.h"

class DataRef {

	protected:
		bool changed;
		XPLMDataRef ptr;
		std::string ser_code;
		virtual const std::string getValStr() =0;
		
	public:
		DataRef(): changed(false), ptr(NULL) {}
		void assign(const std::string name, const std::string ser_code);
		bool isChanged() { return changed; };
		bool isInit() { return (ptr!=NULL); }
		virtual void update() =0;
		const std::string getSerStr();
		
};

/* ******************************************************************* */

class DataRefInt : public DataRef {

	protected:
		int last;
		const std::string getValStr();
  
	public:
		DataRefInt(): last(0), DataRef() {}
		DataRefInt(const std::string name, const std::string ser_code): DataRef(), last(0) { assign(name, ser_code); }
		void update();
		const std::string getSerStr();
};

/* ******************************************************************* */

class DataRefIntInt : public DataRefInt {

	protected:
		const std::string getValStr();

	public:
		DataRefIntInt(const std::string name, const std::string ser_code): DataRefInt(name, ser_code) {}
	
};


/* ******************************************************************* */

class DataRefFloat : public DataRef {
	float last;

	protected:
		const std::string getValStr();
  
	public:
		DataRefFloat(): last(0.0), DataRef() {}
		void update();
		const std::string getSerStr();
};


