/****************************************************************************
 * Ink Dispenser RC1 by Erik Fargas  2015                                      *
 ****************************************************************************/

#include <iostream>
#include <linux/input.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <pthread.h>
#include "midas_lcd.h"
#include "GPIO.h"
#include "PWM.h"

#define KEY_PRESS 1
#define KEY_RELEASE 0
#define PULSE_GPIO 38                   ///< Input GPIO for pulses GPIO1_6
#define TRIGGER_PULSES 6                ///< Number of pulses to trigger routine
#define LINE1 1                         ///< LCD ROW1
#define LINE2 2                         ///< LCD ROW2
#define CHAR_IP 7                       ///< LCD IP Character
#define PUMP_GPIO "pwm_test_P9_14.12"   ///< PWM GPIO
#define PUMP_PERIOD 10000               ///< PWM Period in us
#define PUMP_DUTY 50.0f                 ///< PWM Duty %
#define DELAY1 25000                    ///< 25ms Delay
#define DELAY2 50000                    ///< 50ms Delay

using namespace std;
using namespace exploringBB;

int string_to_charArr(string strg, int lcd_line);
int routine(int cycles);
int count_pulses(int var);
int getIP(void);
int printIP(void);
int printIDLE(void);
int printBUSY(void);
int printPulses(void);
int printCycles(void);
int first_press(void);
int pressed_key(int code);
int released_key(int code);
void update_lcd();
int setup_Trigger_gpio(int num_gpio, GPIO_EDGE edge);
int setup_EV1_gpio(int num_gpio);
int setup_EV2_gpio(int num_gpio);
int setup_pump_gpio(string pwm_gpio_name, int period, float duty, int polarity);
int start(void);
int Check_root(void);
int get_input_event(void);

int cycles=1;           //Number of cycles to execute
int fd;
int running=0;
int state=0;
int pulses=0;

GPIO *inTrigger,*outEV1,*outEV2;
PWM *outPump;

guint8 ip[17];
guint8 line1[17];
guint8 line2[17];
string texts[5]={"Press any key...","Status: IDLE","Status: BUSY","CYCL: ","CNT: "};

int routine(int cycles){
    printBUSY();
    for (int i=0; i< cycles; i++){
        cout << "Do something" << endl;
        outPump->setDutyCycle(50.0f);
        //outPump->run();
        outEV1->streamWrite(HIGH);
        outEV2->streamWrite(HIGH);
        usleep(DELAY1);
        outEV1->streamWrite(LOW);
        outEV2->streamWrite(LOW);
        //outPump->stop();
        outPump->setDutyCycle(0.0f);
        usleep(DELAY2);
    }
    printIDLE();
    printCycles();
    printPulses();
    return 0;
}

int count_pulses(int var){
    if (pulses < TRIGGER_PULSES){
        pulses += 1;
        printPulses();
        usleep(5000);
        cout << pulses << endl;
        if (pulses == TRIGGER_PULSES){
            routine(cycles);
            pulses = 0;
        }
    }
    return 0;
}

int getIP(void){
    char s[17];
    FILE * fp = popen("ifconfig", "r");
    if (fp) {
        char *p=NULL, *e; size_t n;
        while ((getline(&p, &n, fp) > 2) && p) {
            if (p = strstr(p, "inet ")){
                p+=5;
                if (p = strchr(p, ':')){
                    ++p;
                    if (e = strchr(p, ' ')){
                        *e='\0';
                    strcpy(s,p);
                    }
                }
            }
        }
    }
    for (int i=0; i<=16; i++){
        ip[i]=s[i];
    }
    pclose(fp);
    return 0;
}

int printIP(void){
    getIP();
    LCD_CLS();
    Gotoxy(1,0);
    SendByte(iDat,CHAR_IP);
    Gotoxy(1,1);
    SendStr(ip);
    return 0;
}

int printCycles(void){
    Gotoxy(2,0);
    string_to_charArr(texts[3],LINE2);
    bool skip = false;
    char* buffer= new char[6];
    sprintf(buffer, "%d", cycles);
    if (cycles == 9){
        skip=true;
    }
    for (int i=6; i<=8;i++){
        if (skip == false){
            line2[i]=buffer[i-6];
        }
        if (skip == true){
            line2[i]=buffer[i-6];
            buffer[i-5]=' ';
            skip = false;
        }
    }
    delete[] buffer;
    SendStr(line2);
    return 0;
}

int printPulses(void){
    Gotoxy(2,9);
    string_to_charArr(texts[4],LINE2);
    bool skip = false;
    char* buffer= new char[5];
    sprintf(buffer, "%d", pulses);
    if (pulses < 10){
        skip=true;
    }
    for (int i=14; i<=17;i++){
        if (skip == false){
            line2[i-9]=buffer[i-14];
        }
        if (skip == true){
            line2[i-9]=buffer[i-14];
            buffer[i-13]=' ';
            skip = false;
        }
    }
    delete[] buffer;
    SendStr(line2);
    return 0;
}

int printIDLE(void){
    Gotoxy(1,0);
    string_to_charArr(texts[1],LINE1);
    SendStr(line1);
    return 0;
}

int printBUSY(void){
    Gotoxy(1,0);
    string_to_charArr(texts[2],LINE1);
    SendStr(line1);
    return 0;
}

int string_to_charArr(string strg, int lcd_line){
    std::stringstream ss;
    ss.str (strg);
    std::string s = ss.str();
    int i = 0;
    if (lcd_line == 1){
        for (i; i<=17; i++){
            line1[i]=s[i];
        }
    }
    if (lcd_line == 2){
        for (i; i<=17; i++){
            line2[i]=s[i];
        }
    }
    //copy empty string into ss
    ss.str(std::string());
    //clears EOF flag
    ss.clear();
    return 0;
}

int first_press(void){
    LCD_CLS();
    printIDLE();
    printCycles();
    printPulses();
    return 1;
}

int pressed_key(int code){
    if (code == 85) {
        if (state == 0){
            state = first_press();
            return 0;
        }
        if (cycles < 30){
            cycles +=1;
        }
        printCycles();
        printPulses();
    }
    else if (code == 68) {
        if (state == 0){
            state = first_press();
            return 0;
        }
        if (cycles > 1){
            cycles -=1;
        }

        printCycles();
        printPulses();
    }
    else if (code == 76) {
        if (state == 0){
            state = first_press();
        }
        routine(cycles);
    }
    else if (code == 82) {
        if (state == 0){
            state = first_press();
        }
        printIP();
    }
    return 0;
}

int released_key(int code){
    if (code == 85) {
       update_lcd();
    }
    else if (code == 68) {
        update_lcd();
    }
    else if (code == 76) {
        update_lcd();
    }
    else if (code == 82) {
        update_lcd();
    }
    return 0;
}

void update_lcd(){
    LCD_CLS();
    printIDLE();
    printCycles();
    printPulses();
}

int setup_Trigger_gpio(int num_gpio, GPIO_EDGE edge){
    inTrigger = new GPIO(num_gpio);
    inTrigger->setDirection(INPUT);
    inTrigger->setEdgeType(edge);
    return 0;
}

int setup_EV1_gpio(int num_gpio){
    outEV1 = new GPIO(num_gpio);
    outEV1->setDirection(OUTPUT);
    outEV1->streamOpen();
    return 0;
}

int setup_EV2_gpio(int num_gpio){
    outEV2 = new GPIO(num_gpio);
    outEV2->setDirection(OUTPUT);
    outEV2->streamOpen();
    return 0;
}

int setup_pump_gpio(string pwm_gpio_name, int period, float duty, int polarity){
    outPump = new PWM(pwm_gpio_name);   // P9_14 MUST be loaded as a slot before use
    outPump->setPeriod(period);         // Set the period in ns 1/1KHz = 1ms = 1000us
    outPump->setDutyCycle(duty);        // Set the duty cycle as a percentage
    if (polarity == 1){
        outPump->setPolarity(PWM::ACTIVE_HIGH);  // using active high PWM
    }
    else {
        outPump->setPolarity(PWM::ACTIVE_LOW);   // using active low PWM
    }
    return 0;
}

int start(void){
    //printIP();
    Gotoxy(2,0);
    string_to_charArr(texts[0],LINE2);
    SendStr(line2);
    setup_Trigger_gpio(PULSE_GPIO,FALLING);
    setup_EV1_gpio(27);
    setup_EV2_gpio(45);
    setup_pump_gpio(PUMP_GPIO,PUMP_PERIOD,0.0f,0);
    outPump->run();
    inTrigger->waitForEdge(&count_pulses);
    return 0;
}

int Check_root(void){
    if(getuid()!=0){
        cout << "You must run this program as root. Exiting." << endl;
        exit(2);
    }
    return 0;
}

int get_input_event(void){
    if ((fd = open("/dev/input/event1", O_RDONLY)) < 0){
        perror("Failed to open event1 input device. Exiting.");
        exit(2);
    }
    return 1;
}

int main(){
    cout << "Starting Dispenser" << endl;
    Check_root();
    LCD_Initial();

    struct input_event event[64];
    running = get_input_event();

    if (running == 1){
        cout << "Dispenser initialized" << endl;
        start();
    }

    while(running==1){  // Press and Release are one loop each

        int numbytes = (int)read(fd, event, sizeof(event));
        if (numbytes < (int)sizeof(struct input_event)){
            perror("The input read was invalid. Exiting.");
            return -1;
        }

        for (int i=0; i < numbytes/sizeof(struct input_event); i++){
            int type = event[i].type;
            int val  = event[i].value;
            int code = event[i].code;
            if (type == EV_KEY) {
                if (val == KEY_PRESS){
                    pressed_key(code);
                    cout << "Press  : Code "<< code <<" Value "<< val<< endl;
                }
                if (val == KEY_RELEASE){
                    cout << "Release: Code "<< code <<" Value "<< val<< endl;
                    released_key(code);
                }
            }
        }
    }
    close(fd);
    return 0;
}
