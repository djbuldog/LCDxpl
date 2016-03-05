/* Inspired by:
 * https://github.com/jmasnik/ArduinoXPL
 */

#include <LiquidCrystal.h>
#include <Encoder.h>

LiquidCrystal lcd(A3, A2, 14, 15, A0, A1);
Encoder knob_down(9,8);
Encoder knob_up(7,6);

#define SER_BUF_SIZE  25

uint8_t editval = 0;
uint8_t btn1 = 1;
uint8_t btn2 = 1;
uint8_t btn3 = 1;
unsigned long last_btn_millis = 0; //btn debauncing
int8_t last_knob_up = 0;
int8_t last_knob_down = 0;
char    serbuf[SER_BUF_SIZE];
uint8_t serpos;

#define ADF_LIMIT   0
#define NAV_LIMIT   1
#define COM_LIMIT   2
#define DEG_LIMIT   3
#define SQK_LIMIT   4

const struct s_data_limits {
  uint16_t  max; // max number
  uint8_t   min; // min number/digit
  uint8_t  maxd; // max digit, 0 = no check
} dlimits[] = { { 999, 0, 9 },    // ADF // 190-535?
                { 117, 108, 0 },  // NAV
                { 136, 118, 0 },  // COM
                { 359, 0, 0 },    // DEG
                { 7777, 0, 7 }};  // SQK

struct s_nav_data {
  uint16_t adf;
  uint16_t squawk;    
  uint8_t com1_use_a;
  uint8_t com1_use_b;
  uint8_t com1_sby_a;
  uint8_t com1_sby_b;
  uint8_t nav1_use_a;
  uint8_t nav1_use_b;
  uint8_t nav1_sby_a;
  uint8_t nav1_sby_b;
} navdata = { dlimits[ADF_LIMIT].min, 0, 
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
                   {NULL, OP_NONE, 0, 0} };
/*
const struct s_editvals {
  uint16_t inc1;
  uint8_t inc2;
  uint8_t idpar;
} editvals[] = { {0, 0, 6},      //NULL
                 {1, 2, 1},      //COM STBY
                 {1, 5, 3},      //NAV STBY
                 {100, 10, 4},   //ADF high
                 {10, 1, 4},     //ADF low
                 {1000, 100, 5}, //SQK high
                 {10, 1, 5} };   //SQK low
*/
const struct s_editvals {
  uint16_t inc1;
  uint8_t inc2;
  uint8_t idpar;
} editvals[] = { {0, 0, 6},      //NULL
                 {1, 2, 0},      //COM STBY
                 {1, 5, 2},      //NAV STBY
                 {100, 10, 4},   //ADF high
                 {10, 1, 4},     //ADF low
                 {1000, 100, 5}, //SQK high
                 {10, 1, 5} };   //SQK low


/*
 * dava rotacni
 * cudl na rot1 by vybiral dalsi element
 * cudl na rot2 by vybiral predchozi element
 * otaceni by menilo hodnotu
 * elemetny by byly com1, nav1, squawkAB, squawkCD, ..
 * special tlacitko.. u com/nav prehozeni hodnoty na aktivni
 * u squawk/adf funkce zadna...
 * 
 * na GPS screen by rot enkodery mohly fungovat jako ty dva ovladace
 * oba rot cudl funkce cudlu na gps?
 * special tlacitko bez funkce?
 * 
 * jeste potrebuji alespon jedno tlacitko na zmenu screenu
 * nebo vyuzit to special? kdyz bude vypnuty edit rezim, tak by prepinal? :)
 * funkce
 * - pokud nav/com.. prehod use za stby
 * - pokud adf/squawk.. tak zrus edit mode....
 * - pokud nebude v edit mode, tak zmen obrazovku
 * 
 * alt screen
 * jeden rot +1 QNH, druhy +10
 * 
 * AP screen....
 * jeste by bylo dobre kontrolovat letadlo... a pokud letadlo pristroj nema (gps treba), tak by nebylo k dispozici
 */

/*
const char *sc[][4] = { // ------ screen 0 ---- 
                        { "COM1 000.000  SQUAWK",
                          "COM2 000.000 0 0 0 0",
                          "NAV1 000.000 AD1 000",
                          "NAV2 000.000 AD2 000" },
                        // ------ screen 1 ---- 
                        { "COM1 000.00 (000.00)",
                          "NAV1 000.00 (000.00)",
                          "    ** SQUAWK **    ",
                          "      0 7 7 7       " },
                        // ------ screen 1 ----
                        { "COM1 000.00 (000.00)",
                          "NAV1 000.00 (000.00)",
                          "     ADF  0 0 0     ",
                          "   QUAWK  0 7 7 7   " },
                        // ------ screen 1 ----
                        { "    COM1  000.00    ",
                          "    NAV1  000.00    ",
                          "     ADF  0 0 0     ",
                          "   QUAWK  0 7 7 7   " },
                        // ------ screen 1 ----
                        { "     COM  NAV       ",
                          "  000.00  000.00    ",
                          "     ADF  0 0 0     ",
                          "   QUAWK  0 7 7 7   " },
                        // ------ screen 1 ---- 
                        { "        QNH         ",
                          "  1022 hpa / 2992   ",
                          "       SQUAWK       ",
                          "      0 7 7 7       " },
                        // ------ screen GPS---
                        { "LKTB [. .#.| |. . .]",
                          "DIS 51.10    DTK 98 ",
                          "GS  112      DBG 97 ",
                          "ETE 27:80    TRK 238" },
                        };

void draw_static(uint8_t screen) {
  for (uint8_t i=0; i<4; ++i) {
    lcd.setCursor(0,i);
    lcd.print(sc[screen][i]);
  }
}
*/
/*
void draw_naw_screen(uint8_t seledit) {
  char screen[4][21];
  char adf_squawk[3+4+1];
  char lr[] = "()()        ";

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
*/
void draw_naw_screen(uint8_t seledit) {
  char screen[4][21];
  char adf_squawk[3+4+1];
  char lr[] = "            ";

  if (seledit--) {
    lr[seledit*2]='[';
    lr[seledit*2+1]=']';
    if (seledit==5) lr[9]='[';
  }

  sprintf(&adf_squawk[0],"%03hu%04u", navdata.adf, navdata.squawk);

  sprintf(&screen[0][0],"C152  COM %c%03hu.%02hu%c  ", 
          lr[0], navdata.com1_use_a, navdata.com1_use_b, lr[1]);
  sprintf(&screen[1][0],"      NAV %c%03hu.%02hu%c  ",
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


void setup()
{
  Serial.begin(9600); 
  lcd.begin(20,4);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  draw_naw_screen(0);
  serpos=0;
  serbuf[serpos]='\0';
}

void loop() {
  char c;
  int8_t tmp1, tmp2;

  if ((millis()-last_btn_millis)>100) {

  if (digitalRead(2) == LOW) {
    if (btn1) {
      if (editval==1) {
/*        tmp1 = navdata.com1_use_a;
        navdata.com1_use_a = navdata.com1_sby_a;
        navdata.com1_sby_a = tmp1;
        tmp1 = navdata.com1_use_b;
        navdata.com1_use_b = navdata.com1_sby_b;
        navdata.com1_sby_b = tmp1;
*/        Serial.println("C1X");
      } else if (editval==2) {
/*        tmp1 = navdata.nav1_use_a;
        navdata.nav1_use_a = navdata.nav1_sby_a;
        navdata.nav1_sby_a = tmp1;
        tmp1 = navdata.nav1_use_b;
        navdata.nav1_use_b = navdata.nav1_sby_b;
        navdata.nav1_sby_b = tmp1;
*/        Serial.println("N1X");
      } else {
        editval=0;
      }
      draw_naw_screen(editval);
      btn1 = 0;
      last_btn_millis=millis();
    }
  } else { 
      btn1 = 1; 
      last_btn_millis=millis();
  }

  if (digitalRead(3) == LOW) {
    if (btn2) {
      editval=(editval+1)%7;
      draw_naw_screen(editval);
      btn2 = 0;
      last_btn_millis=millis();
    }
  } else { 
    btn2 = 1; 
    last_btn_millis=millis();
  }

  if (digitalRead(5) == LOW) {
    if (btn3) {
      editval=(editval)?(editval-1)%7:6;
      draw_naw_screen(editval);
      btn3 = 0;
      last_btn_millis=millis();
    }
  } else { 
    btn3 = 1; 
    last_btn_millis=millis();
  }

  if (!btn3 && !btn2) {
    // try to fix LCD screen
    lcd.begin(20,4);

    // just signalization :)
    delay(200);
    draw_naw_screen(editval);
  }
    
  }

  tmp1 = knob_up.read();
  tmp2 = knob_down.read();

//  if ((tmp1 != last_knob_up) || (tmp2 != last_knob_down)) {
  if ( ((tmp1-last_knob_up)>3) || ((tmp2-last_knob_down)>3) ||
       ((tmp1-last_knob_up)<-3) || ((tmp2-last_knob_down)<-3) ) {

    struct s_ser_parser *sp_ptr;
    sp_ptr = const_cast<s_ser_parser*>(&ser_parser[editvals[editval].idpar]);
    uint8_t offset;
    int16_t inc;

    if (tmp1 != last_knob_up) {
/*      Serial.write("knob up: ");
      Serial.println(tmp1);*/
      knob_up.write(0);
      last_knob_up = 0;
      offset = sp_ptr->offset1;
      inc = (editvals[editval].inc1)*(tmp1/4);
    }

    if (tmp2 != last_knob_down) {
/*      Serial.write("knob down: ");
      Serial.println(tmp2);*/
      knob_down.write(0);
      last_knob_down = 0;
      offset = sp_ptr->offset2;
      inc = (editvals[editval].inc2)*(tmp2/4);
    }

    if (sp_ptr->op == OP_INT) {
      uint16_t *val;
      val = (uint16_t*)((uint8_t*)&navdata + offset);


      if (dlimits[sp_ptr->idlim].maxd) {

        int8_t num = ((*val)/((inc<0)?-inc:inc))%10;
        if ( ((num==dlimits[sp_ptr->idlim].maxd) && (inc>0)) || 
             ((num==dlimits[sp_ptr->idlim].min) && (inc<0))) {          
          inc = -(dlimits[sp_ptr->idlim].maxd*inc);
        }
      }
             
      *val += inc;

      if (!dlimits[sp_ptr->idlim].maxd) {
        if ( ((*val) < dlimits[sp_ptr->idlim].min) ||
             ((*val) > dlimits[sp_ptr->idlim].max)) {
          *val-=inc;
        }          
      }

      Serial.print(sp_ptr->keyword);
      Serial.println(*val);
/*      Serial.print(offset);
      Serial.print("<- offset, val ptr->");
      Serial.println((long)val);*/
    }
    
    if (sp_ptr->op == OP_INTINT) {
      uint8_t *val;
      val = (uint8_t*)&navdata + offset;
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
      
/*      Serial.print(offset);
      Serial.print("<- offset, val ptr->");
      Serial.println((long)val);*/
    }

    if (sp_ptr->op != OP_NONE) {
//      Serial.println("refreshing screen\n");
      draw_naw_screen(editval);
    }
  }  

  if(Serial.available() > 0) {

    c = Serial.read();
    
    if (c==10) {

      Serial.write("Received: ");
      Serial.write(serbuf);
      Serial.write('\n');

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
        draw_naw_screen(editval);
      }

      serpos=0;
      serbuf[serpos]='\0';

    } else {
/*
      struct s_ser_parser *sp_ptr;
      sp_ptr = const_cast<s_ser_parser*>(&ser_parser[editvals[editval].idpar]);
      uint8_t offset;
      int16_t inc;

      if (c=='*') {
        offset = sp_ptr->offset1;
        inc = editvals[editval].inc1;
      }
      if (c=='+') {
        offset = sp_ptr->offset2;
        inc = editvals[editval].inc2;
      }
      if (c=='/') {
        offset = sp_ptr->offset1;
        inc = -editvals[editval].inc1;
      }
      if (c=='-') {
        offset = sp_ptr->offset2;
        inc = -editvals[editval].inc2;
      }

      if (sp_ptr->op == OP_INT) {
        uint16_t *val;
        val = (uint16_t*)((uint8_t*)&navdata + offset);
        *val += inc;
          
        if ((*val) < dlimits[sp_ptr->idlim].min)
          *val-=inc;
        if ((*val) > dlimits[sp_ptr->idlim].max)
          *val-=inc;
      }
    
      if (sp_ptr->op == OP_INTINT) {
        uint8_t *val;
        val = (uint8_t*)&navdata + offset;
        *val += inc;

        if ((*val) < dlimits[sp_ptr->idlim].min)
          *val=dlimits[sp_ptr->idlim].max;
        if ((*val) > dlimits[sp_ptr->idlim].max)
          *val=dlimits[sp_ptr->idlim].min;          
      }

      if (sp_ptr->op != OP_NONE) {
        Serial.println("refreshing screen due to val update");
        draw_naw_screen(editval);
      } else {
*/        if (serpos<SER_BUF_SIZE) {
          serbuf[serpos++]=c;
          serbuf[serpos]='\0';
        }
//      }
      
    }
  }
  
  //delay(10);

}
