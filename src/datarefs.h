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
		virtual void update(std::string command) =0;
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
		void update(std::string command);
		const std::string getSerStr();
};

/* ******************************************************************* */

class DataRefIntInt : public DataRefInt {

	protected:
		const std::string getValStr();

	public:
		DataRefIntInt(const std::string name, const std::string ser_code): DataRefInt(name, ser_code) {}
		void update(std::string command);

};


/* ******************************************************************* */

class DataRefFloat : public DataRef {
	float last;

	protected:
		const std::string getValStr();

	public:
		DataRefFloat(): last(0.0), DataRef() {}
		DataRefFloat(const std::string name, const std::string ser_code): DataRef(), last(0.0) { assign(name, ser_code); }
		void update(std::string command);
		const std::string getSerStr();
};

/* ******************************************************************* */

class DataRefFloatInt : public DataRef {
	int last;
	int prec;

	protected:
		const std::string getValStr();

	public:
		DataRefFloatInt(): last(0), prec(1), DataRef() {}
		DataRefFloatInt(const std::string name, const std::string ser_code, int prec=1): DataRef(), last(0), prec(prec) { assign(name, ser_code); }
		void update(std::string command);
		const std::string getSerStr();
};


