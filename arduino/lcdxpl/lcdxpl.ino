/* 
 * Inspired by:
 * https://github.com/jmasnik/ArduinoXPL
 *   
 * Comparing structure vs class driven screen code
 *   
 *   struct driven:
 *   Size 12 866 B (44%) of 28 672 B
 *   Global variables 591 B (23%), 1 969 B for locals of 2 560 B
 *   
 *   class driven:
 *   Size 12 952 B (45%) of 28 672 B
 *   Global variables 602 B (23%), 1 958 B for locals of 2 560 B
 * 
 */

#include <LiquidCrystal.h>
#include <Encoder.h>

LiquidCrystal lcd(A3, A2, 10, 16, 14, 15);
Encoder knob_down(9,3);
Encoder knob_up(2,6);

uint8_t btn1 = 1;
uint8_t btn2 = 1;
uint8_t btn3 = 1;
uint8_t btn4 = 1;
uint8_t sw1 = HIGH;
uint8_t sw2 = HIGH;
uint8_t sw3 = HIGH;
unsigned long last_btn_millis = 0; //btn debouncing
int8_t last_knob_up = 0;
int8_t last_knob_down = 0;
int8_t last_screen = 0;

#define SER_BUF_SIZE  25

char    serbuf[SER_BUF_SIZE];
uint8_t serpos;

#define ADF_LIMIT   0
#define NAV_LIMIT   1
#define COM_LIMIT   2
#define DEG_LIMIT   3
#define SQK_LIMIT   4
#define MAX_LIMIT   5

const struct s_data_limits {
  uint16_t  max; // max number
  uint8_t   min; // min number/digit
  uint8_t  maxd; // max digit, 0 = no check
} dlimits[] = { { 999, 0, 9 },    // ADF // 190-535?
                { 117, 108, 0 },  // NAV
                { 136, 118, 0 },  // COM
                { 359, 0, 0 },    // DEG
                { 7777, 0, 7 },   // SQK
                { 65535, 0, 0 }}; // MAX

struct s_nav_data {
  uint16_t adf;
  uint16_t squawk;
  uint16_t dis;
  uint16_t gs;
  uint16_t ete;
  uint16_t dtk;
  uint16_t dbg;
  uint16_t trk;
  uint8_t com1_use_a;
  uint8_t com1_use_b;
  uint8_t com1_sby_a;
  uint8_t com1_sby_b;
  uint8_t nav1_use_a;
  uint8_t nav1_use_b;
  uint8_t nav1_sby_a;
  uint8_t nav1_sby_b;
} navdata = { dlimits[ADF_LIMIT].min, 0, 0, 0, 0, 
              dlimits[DEG_LIMIT].min, dlimits[DEG_LIMIT].min, 
              dlimits[DEG_LIMIT].min,
              dlimits[COM_LIMIT].min, 0, dlimits[COM_LIMIT].min, 0, 
              dlimits[NAV_LIMIT].min, 0, dlimits[NAV_LIMIT].min, 0};

#define OP_INT    0
#define OP_INTINT 1
#define OP_FLOAT  2
#define OP_STR4   3
#define OP_NONE   254

#define OFFSETOF(x) offsetof(struct s_nav_data, x)

const struct s_ser_parser {
  char *keyword;
  uint8_t op;
  uint8_t idlim;
  uint8_t offset1;
  uint8_t offset2;
} ser_parser[] = { {"C1u" , OP_INTINT, COM_LIMIT,
                      OFFSETOF(com1_use_a), OFFSETOF(com1_use_b)},
                   {"C1s" , OP_INTINT, COM_LIMIT,
                      OFFSETOF(com1_sby_a), OFFSETOF(com1_sby_b)},
                   {"N1u" , OP_INTINT, NAV_LIMIT,
                      OFFSETOF(nav1_use_a), OFFSETOF(nav1_use_b)},
                   {"N1s" , OP_INTINT, NAV_LIMIT,
                      OFFSETOF(nav1_sby_a), OFFSETOF(nav1_sby_b)},
                   {"ADF" , OP_INT, ADF_LIMIT,
                      OFFSETOF(adf), OFFSETOF(adf)},
                   {"SQK" , OP_INT, SQK_LIMIT,
                      OFFSETOF(squawk), OFFSETOF(squawk)},
                   {"DIS" , OP_INT, MAX_LIMIT,
                      OFFSETOF(dis), OFFSETOF(dis)},
                   {"GSp" , OP_INT, MAX_LIMIT,
                      OFFSETOF(gs), OFFSETOF(gs)},
                   {"ETE" , OP_INT, MAX_LIMIT,
                      OFFSETOF(ete), OFFSETOF(ete)},
                   {"DTK" , OP_INT, DEG_LIMIT,
                      OFFSETOF(dtk), OFFSETOF(dtk)},
                   {"DBG" , OP_INT, DEG_LIMIT,
                      OFFSETOF(dbg), OFFSETOF(dbg)},
                   {"TRK" , OP_INT, DEG_LIMIT,
                      OFFSETOF(trk), OFFSETOF(trk)},
                   {NULL, OP_NONE, 0, 0} };

/*
int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
*/
class BaseScreen {
  
  public:
    virtual void redraw()=0;
    virtual void handleBtnPress(uint8_t btnid)=0;
    virtual void handleRotEnStep(uint8_t rotid, int8_t val)=0;
  
};

class EditableScreen : public BaseScreen {

  private:
    const uint8_t maxeditval;

  protected:
    uint8_t editval;

    struct s_editvals {
      uint16_t inc1;
      uint8_t inc2;
      uint8_t idpar;
    };
    
    void modifyVal(const s_editvals editvals[], bool first, int8_t step);
  
  public:
    EditableScreen(uint8_t maxeditval): maxeditval(maxeditval), editval(0) {}
    void nextEditval() { changeEditval(1); };
    void prevEditval() { changeEditval(-1); };
    void resetEditval() { editval=0; redraw(); };
    void changeEditval(int8_t val) { editval=(editval||val>0)?(editval+val)%maxeditval:maxeditval-1; redraw(); };
};

void EditableScreen::modifyVal(const s_editvals editvals[], bool first, int8_t step) {

  struct s_ser_parser *sp_ptr;
  sp_ptr = const_cast<s_ser_parser*>(&ser_parser[editvals[editval].idpar]);
  uint8_t offset;
  int16_t inc;

  if (first) {

    offset = (!editvals[editval].inc2)?sp_ptr->offset2:sp_ptr->offset1;
    inc = (editvals[editval].inc1);

  } else {

    offset = sp_ptr->offset2;
    inc = (editvals[editval].inc2);

  }

  if (sp_ptr->op == OP_INT) {
    uint16_t *val;
    val = (uint16_t*)((uint8_t*)&navdata + offset);

    if (dlimits[sp_ptr->idlim].maxd) {

      int8_t num, num2;
      
      num = ((*val)/inc)%10;
      num2 = num + step - dlimits[sp_ptr->idlim].min;
      num2 %= dlimits[sp_ptr->idlim].maxd + 1 - dlimits[sp_ptr->idlim].min;
      if (num2<0) num2 += dlimits[sp_ptr->idlim].maxd + 1;
      num2 += dlimits[sp_ptr->idlim].min;

      *val += (num2-num)*inc;
      
    } else {

      inc*=step;
      *val+=inc;
      if ( ((*val) < dlimits[sp_ptr->idlim].min) ||
           ((*val) > dlimits[sp_ptr->idlim].max)) {
        *val-=inc;
      }
      
    }

    Serial.print(sp_ptr->keyword);
    Serial.println(*val);
  }
  
  if (sp_ptr->op == OP_INTINT) {
    uint8_t *val;
    val = (uint8_t*)&navdata + offset;

    inc *= step;
    *val += inc;

    if (offset==sp_ptr->offset1) {
      if ((*val) < dlimits[sp_ptr->idlim].min)
        *val=dlimits[sp_ptr->idlim].max;
      if ((*val) > dlimits[sp_ptr->idlim].max)
        *val=dlimits[sp_ptr->idlim].min;
    } else {
      if (*val > 99) {
        *val=(inc>0)?0:100+inc;
      }
    }

    Serial.print(sp_ptr->keyword);
    val = (uint8_t*)&navdata + sp_ptr->offset1;
    Serial.print(*val);
    Serial.print(".");
    val = (uint8_t*)&navdata + sp_ptr->offset2;
    if (*val<10) Serial.print("0");
    Serial.println(*val);
    
  }

  if (sp_ptr->op != OP_NONE) {
//      Serial.println("refreshing screen\n");
    redraw();
  }
}

/* ****************************************************************** 
 *  screen&control schema 3
 *   - C152 .. COM, NAV, ADF, SQK
 *   - down rot = select editval
 *   - up rot = change val
 *   
*/

class Screen3 : public EditableScreen {

  private:
    static const s_editvals editvals[];

  public:
    Screen3(): EditableScreen(12) {}
    void redraw();
    void handleBtnPress(uint8_t btnid);
    void handleRotEnStep(uint8_t rotid, int8_t val);
  
};

const Screen3::s_editvals 
      Screen3::editvals[] =  { {0, 255, 12},     //NULL
                               {1, 255, 0},      //COM USE high
                               {2, 0, 0},        //COM USE low
                               {1, 255, 2},      //NAV USE high
                               {5, 0, 2},        //NAV USE low
                               {100, 255, 4},    //ADF fst num
                               {10, 255, 4},     //ADF snd num
                               {1, 255, 4},      //ADF rd num
                               {1000, 255, 5},   //SQK fst num
                               {100, 255, 5},    //SQK snd num
                               {10, 255, 5},     //SQK rd num
                               {1, 255, 5} };    //SQK th num

void Screen3::redraw() {
  char screen[4][21];
  char adf_squawk[3+4+1];
  char lr[] = " .  .          ";
  uint8_t seledit = editval;

  if (seledit--) {
    uint8_t i = 0;
    if(seledit==6) ++i;
    if(seledit>7) ++i;
    if(seledit==10) ++i;
    lr[seledit+(seledit/2)-i]='[';
    lr[seledit+(seledit/2)-i+1]=']';
  }

  sprintf(&adf_squawk[0],"%03hu%04u", navdata.adf, navdata.squawk);

  sprintf(&screen[0][0],"C152  COM %c%03hu%c%02hu%c  ", 
          lr[0], navdata.com1_use_a, lr[1], navdata.com1_use_b, lr[2]);
  sprintf(&screen[1][0],"v2    NAV %c%03hu%c%02hu%c  ",
          lr[3], navdata.nav1_use_a, lr[4], navdata.nav1_use_b, lr[5]);
  sprintf(&screen[2][0],"      ADF %c%c%c%c%c%c%c   ", 
          lr[6], adf_squawk[0], lr[7], adf_squawk[1], lr[8], adf_squawk[2], lr[9]);
  sprintf(&screen[3][0],"   SQUAWK %c%c%c%c%c%c%c%c%c ", 
          lr[10], adf_squawk[3], lr[11], adf_squawk[4], lr[12], 
          adf_squawk[5], lr[13], adf_squawk[6], lr[14]);

  for (uint8_t i=0; i<4; ++i) {
    lcd.setCursor(0,i);
    lcd.print(screen[i]);
  }
}

void Screen3::handleBtnPress(uint8_t btnid) {
  if (btnid==0) resetEditval();
  if (btnid==1) nextEditval();
  if (btnid==2) prevEditval(); 
}

void Screen3::handleRotEnStep(uint8_t rotid, int8_t val) {
  if (rotid==0) changeEditval(val);
  if (rotid==1) modifyVal(editvals, true, val);
}

/* ****************************************************************** 
 *  screen&control schema 2
 *   - C152 .. COM, NAV, ADF, SQK
 *   - down rot = change snd val
 *   - up rot = change fst val
 *   - down rot btn = select next editval
 *   - down rot btn = select prev editval
*/

class Screen2 : public EditableScreen {

  private:
    static const s_editvals editvals[];

  public:
    Screen2(): EditableScreen(7) {}
    void redraw();
    void handleBtnPress(uint8_t btnid);
    void handleRotEnStep(uint8_t rotid, int8_t val);
  
};

const Screen2::s_editvals 
      Screen2::editvals[] = { {0, 0, 12},     //NULL
                              {1, 2, 0},      //COM USE
                              {1, 5, 2},      //NAV USE
                              {100, 10, 4},   //ADF high
                              {10, 1, 4},     //ADF low
                              {1000, 100, 5}, //SQK high
                              {10, 1, 5} };   //SQK low
      
void Screen2::redraw() {
  
  char screen[4][21];
  char adf_squawk[3+4+1];
  char lr[] = "            ";
  uint8_t seledit = editval;

  if (seledit--) {
    lr[seledit*2]='[';
    lr[seledit*2+1]=']';
    if (seledit==5) lr[9]='[';
  }

  sprintf(&adf_squawk[0],"%03hu%04u", navdata.adf, navdata.squawk);

  sprintf(&screen[0][0],"C152  COM %c%03hu.%02hu%c  ", 
          lr[0], navdata.com1_use_a, navdata.com1_use_b, lr[1]);
  sprintf(&screen[1][0],"v1    NAV %c%03hu.%02hu%c  ",
          lr[2], navdata.nav1_use_a, navdata.nav1_use_b, lr[3]);
  sprintf(&screen[2][0],"      ADF %c%c%c%c%c%c%c   ", 
          lr[4], adf_squawk[0], lr[6], adf_squawk[1], lr[5], adf_squawk[2], lr[7]);
  sprintf(&screen[3][0],"   SQUAWK %c%c %c%c%c %c%c ", 
          lr[8], adf_squawk[3], adf_squawk[4], lr[9], adf_squawk[5], adf_squawk[6], lr[11]);

  for (uint8_t i=0; i<4; ++i) {
    lcd.setCursor(0,i);
    lcd.print(screen[i]);
  }
  
}

void Screen2::handleBtnPress(uint8_t btnid) {
  if (btnid==0) resetEditval();
  if (btnid==1) nextEditval();
  if (btnid==2) prevEditval(); 
}

void Screen2::handleRotEnStep(uint8_t rotid, int8_t val) {
  if (rotid==0) modifyVal(editvals, true, val);
  if (rotid==1) modifyVal(editvals, false, val);
}

/* ******************************************************************
 *   screen&control schema 1
 *   - COM1, COM1 stby, NAV1, NAV1 stby, ADF, SQK
 *   - down rot = change snd val
 *   - up rot = change fst val
 *   - down rot btn = select next editval
 *   - down rot btn = select prev editval
 *   - black btn = command switch use - stby
*/

class Screen1 : public EditableScreen {

  private:
    static const s_editvals editvals[];

  public:
    Screen1(): EditableScreen(7) {}
    void redraw();
    void handleBtnPress(uint8_t btnid);
    void handleRotEnStep(uint8_t rotid, int8_t val);
  
};

const Screen1::s_editvals 
      Screen1::editvals[] = { {0, 0, 12},     //NULL
                              {1, 2, 1},      //COM STBY
                              {1, 5, 3},      //NAV STBY
                              {100, 10, 4},   //ADF high
                              {10, 1, 4},     //ADF low
                              {1000, 100, 5}, //SQK high
                              {10, 1, 5} };   //SQK low
      
void Screen1::redraw() {
  
  char screen[4][21];
  char adf_squawk[3+4+1];
  char lr[] = "()()        ";
  uint8_t seledit = editval;

  if (seledit--) {
    lr[seledit*2]='[';
    lr[seledit*2+1]=']';
    if (seledit==5) lr[9]='[';
  }

  sprintf(&adf_squawk[0],"%03hu%04u", navdata.adf, navdata.squawk);

  sprintf(&screen[0][0],"COM1 %03hu.%02hu %c%03hu.%02hu%c", navdata.com1_use_a, 
          navdata.com1_use_b, lr[0], navdata.com1_sby_a, navdata.com1_sby_b, lr[1]);
  sprintf(&screen[1][0],"NAV1 %03hu.%02hu %c%03hu.%02hu%c", navdata.nav1_use_a, 
          navdata.nav1_use_b, lr[2], navdata.nav1_sby_a, navdata.nav1_sby_b, lr[3]);
  sprintf(&screen[2][0],"     ADF %c%c%c%c%c%c%c    ", 
          lr[4], adf_squawk[0], lr[6], adf_squawk[1], lr[5], adf_squawk[2], lr[7]);
  sprintf(&screen[3][0],"  SQUAWK %c%c %c%c%c %c%c  ", 
          lr[8], adf_squawk[3], adf_squawk[4], lr[9], adf_squawk[5], adf_squawk[6], lr[11]);

  for (uint8_t i=0; i<4; ++i) {
    lcd.setCursor(0,i);
    lcd.print(screen[i]);
  }
  
}

void Screen1::handleBtnPress(uint8_t btnid) {
  if (btnid==0) resetEditval();
  if ((btnid==1)||(btnid==3)) nextEditval();
  if ((btnid==2)||(btnid==4)) prevEditval(); 
  if (btnid==5) Serial.println("n1b"); 
  if (btnid==6) Serial.println("c1b");
}

void Screen1::handleRotEnStep(uint8_t rotid, int8_t val) {
  if (rotid==0) modifyVal(editvals, true, val);
  if (rotid==1) modifyVal(editvals, false, val);
}

/* ****************************************************************** 
 *   screen&control schema 4
 *   - GPS
 *   - down rot = command gps right knob big
 *   - up rot = command gps right knob low
 *   - down/up rot btn = command gps enter
*/

class Screen4 : public BaseScreen {
  
  public:
    void redraw();
    void handleBtnPress(uint8_t btnid);
    void handleRotEnStep(uint8_t rotid, int8_t val);
  
};

void Screen4::redraw() {
  
  char screen[4][21];
  char adf_squawk[3+4+1];

  sprintf(&screen[0][0],"******* GPS ********");
  sprintf(&screen[1][0],"DIS %5hu.%1hu  DTK %3hu", navdata.dis/10,
          navdata.dis%10, navdata.dtk);
  sprintf(&screen[2][0],"GS  %5hu.%1hu  DBG %3hu", navdata.gs/10,
          navdata.gs%10, navdata.dbg);
  sprintf(&screen[3][0],"ETE %4hu:%02hu  TRK %3hu", navdata.ete/60,
          navdata.ete%60, navdata.trk);

  for (uint8_t i=0; i<4; ++i) {
    lcd.setCursor(0,i);
    lcd.print(screen[i]);
  }
  
}

void Screen4::handleBtnPress(uint8_t btnid) {
  if (btnid==1) Serial.println("Gb1");
  if (btnid==2) Serial.println("Gb2");
  if (btnid==3) Serial.println("Gb3");
  if (btnid==4) Serial.println("Gb4");
  if (btnid==5) Serial.println("Gb5");
  if (btnid==6) Serial.println("Gb6");
}

void Screen4::handleRotEnStep(uint8_t rotid, int8_t val) {
  if (rotid==0) {
    if (val>0) Serial.println("G1p");
    else Serial.println("G1l");
  }
  if (rotid==1) {
    if (val>0) Serial.println("G2p");
    else Serial.println("G2l");
  }
}

/* ****************************************************************** */

BaseScreen *screen = NULL;

void setup()
{
  Serial.begin(9600); 
  lcd.begin(20,4);
  serpos=0;
  serbuf[serpos]='\0';

  // rotary enc btn pins
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);

  // switch pins
  pinMode(7, INPUT_PULLUP);
  pinMode(8, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);

  // black btn pin
  pinMode(A1, INPUT_PULLUP);

  // analog multibutton pin
  pinMode(A0, INPUT_PULLUP);

  // LCD backlight on/off pin
  pinMode(0, OUTPUT);
  
  screen = new Screen1();
  screen->redraw();
  last_screen = 0;
}

void loop() {
  char c;
  int8_t tmp1, tmp2;
  int a;

  if ((millis()-last_btn_millis)>100) {

    a = analogRead(A0) / 50;
    if (a<5) {
      if (btn4) {
        screen->handleBtnPress(3+a);
        btn4 = 0;
        last_btn_millis=millis();
      }
    } else { 
      btn4 = 1; 
      last_btn_millis=millis();
    }

    if (digitalRead(A1) == LOW) {
      if (btn1) {
        if (screen) delete screen;
        if (last_screen == 0) screen = new Screen4();
        if (last_screen == 1) screen = new Screen1();
        //if (last_screen == 2) screen = new Screen1();
        last_screen = ++last_screen%2;
        screen->redraw();
        //screen->handleBtnPress(0);
        btn1 = 0;
        last_btn_millis=millis();
      }
    } else { 
        btn1 = 1; 
        last_btn_millis=millis();
    }

    if (digitalRead(4) == LOW) {
      if (btn2) {
        screen->handleBtnPress(1);
        btn2 = 0;
        last_btn_millis=millis();
      }
    } else { 
      btn2 = 1; 
      last_btn_millis=millis();
    }

    if (digitalRead(5) == LOW) {
      if (btn3) {
        screen->handleBtnPress(2);
        btn3 = 0;
        last_btn_millis=millis();
      }
    } else { 
      btn3 = 1; 
      last_btn_millis=millis();
    }

    if (digitalRead(1) != sw1) {
      Serial.print(F("sw1"));
      Serial.println(!!sw1);
      digitalWrite(0, sw1);
      sw1 = !sw1;
      last_btn_millis=millis();
    }

    if (digitalRead(7) != sw2) {
      Serial.print(F("sw2"));
      Serial.println(!!sw2);
      sw2 = !sw2;
      last_btn_millis=millis();
    }

    if (digitalRead(8) != sw3) {
      Serial.print(F("sw3"));
      Serial.println(!!sw3);
      sw3 = !sw3;
      last_btn_millis=millis();
    }

    if (!btn3 && !btn2) {
      // try to fix LCD screen
      lcd.begin(20,4);

      // just signalization :)
      delay(200);
      screen->redraw();
    }
    
  }

  tmp1 = knob_up.read();
  tmp2 = knob_down.read();

/*
  static unsigned long last_tx_ready = 0; //detect TX troubles
  if ((millis()>1000)&&(!Serial.availableForWrite())) {
    if ((millis()-last_tx_ready)>1000) {
      lcd.setCursor(0,0);
      lcd.print("TX:");
      lcd.print(Serial.availableForWrite());
      lcd.print(" ");
      void (*reboot)(void) = 0;
      reboot();
    }
  } else {
    last_tx_ready=millis();
  }
*/

  if (((tmp1-last_knob_up)>3) || ((tmp1-last_knob_up)<-3)) {
    knob_up.write(0);
    last_knob_up = 0;
    screen->handleRotEnStep(0,tmp1/4);
  }
  
  if (((tmp2-last_knob_down)>3) || ((tmp2-last_knob_down)<-3)) {
    knob_down.write(0);
    last_knob_down = 0;
    screen->handleRotEnStep(1,tmp2/4);
  }

  if(Serial.available() > 0) {

    c = Serial.read();
    
    if (c==10) {

      Serial.print(F("Received: "));
      Serial.println(serbuf);

      struct s_ser_parser *sp_ptr;
      sp_ptr = const_cast<s_ser_parser*>(&ser_parser[0]);
      while(sp_ptr->keyword) {
        if(strncmp(serbuf, sp_ptr->keyword, 3)==0) {
//          Serial.println(sp_ptr->keyword);
//          Serial.println("match!");
          break;
        }
        sp_ptr++;
      }

      if (sp_ptr->keyword) {

        if (sp_ptr->op == OP_INT) {
          uint16_t *val;
          val = (uint16_t*)((uint8_t*)&navdata + sp_ptr->offset1);
          *val = atoi(&serbuf[3]);

          if ((*val) < dlimits[sp_ptr->idlim].min)
            *val=dlimits[sp_ptr->idlim].min;
          if ((*val) > dlimits[sp_ptr->idlim].max)
            *val=dlimits[sp_ptr->idlim].max;

        }

        if (sp_ptr->op == OP_INTINT) {
          uint8_t *val1, *val2;
          val1 = (uint8_t*)&navdata + sp_ptr->offset1;
          val2 = (uint8_t*)&navdata + sp_ptr->offset2;
          sscanf(&serbuf[3],"%03hhu.%02hhu",val1,val2);

          if ((*val1) < dlimits[sp_ptr->idlim].min)
            *val1=dlimits[sp_ptr->idlim].min;
          if ((*val1) > dlimits[sp_ptr->idlim].max)
            *val1=dlimits[sp_ptr->idlim].max;
          
        }
        
//        Serial.println("refreshing screen due to val update");
        screen->redraw();
      }

      serpos=0;
      serbuf[serpos]='\0';

    } else {
      if (serpos<SER_BUF_SIZE) {
        serbuf[serpos++]=c;
        serbuf[serpos]='\0';
      }      
    }
  }
  
//  delay(10);

}
