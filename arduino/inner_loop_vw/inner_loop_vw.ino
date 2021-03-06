#include "DualVNH5019MotorShield.h"
#include <math.h>
#include <ros.h>
#include <ros/time.h>
#include <tf/tf.h>
#include <tf/transform_broadcaster.h>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/Vector3.h>
#include <geometry_msgs/Vector3Stamped.h>
#include <geometry_msgs/Twist.h>
#include <std_msgs/Int8.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

unsigned long Time=0; // Starting time
unsigned long lastMilli = 0; 
double td = 0.01; // T = 0.01 sec (100 hz)
unsigned long sample_time= td*1000 ; 

double wd ;    // Desired angular speed of COM about ICC(Instantaneous center of curvature)
double vd ;    // Desired longitudinal speed of center of mass
double vdf ;   // Feedback values of V
double wdf ;   // Feedback values of W

double wR;      // present angular speed of right motor
double wL;      // present angular speed of left motor
double wRp=0.0; // previous angular speed right motor
double wLp=0.0; // previous angular speed left motor
double wLn;     // average angular speed (wL + wLp)/2  
double wRn;     // average angular speed (wR + wRp)/2

double Radius = 0.06; // Change it (radius of wheel) 0.045
double Length =0.36; // Change it (distance between wheels) 0.555 0.308

double wdr;       // Desired angular speed of right wheel using wd & vd /  prefilter parameter x_{n+1}
double wdl;       // Desired angular speed of left wheel using wd & vd  / prefilter parameter x_{n+1}
double wdr_p=0;   // prefilter parameter x_{n} for right motor
double wdl_p=0;   // prefilter parameter x_{n} for left motor
double wrf;       // prefilter parameter y_{n+1} for right motor
double wlf;       // prefilter parameter y_{n+1} for left motor
double wrf_p=0;   // prefilter paramter y_{n} for right motor
double wlf_p=0;   // prefilter parameter y_{n} for left motor'

double CR;       // Controller output y_{n+2} Right motor
double CR_p=0;   // Controller output y_{n+1} Right motor
double CR_pp=0;  // Controller output y_{n}   Right motor
double CL;       // Controller output y_{n+2} Left motor
double CL_p=0;   // Controller output y_{n+1} Left motor
double CL_pp=0;  // Controller output y_{n}   Left motor

double Lerror;   // Lerror = wlf(output of prefilter/ reference speed) - wLn.....or... Controller input x_{n+2}
double Lerror_p = 0; // Controller input x_{n+1}
double Lerror_pp = 0; // Controller input x_{n}
double Rerror;   // Rerror = wrf(output of prefilter/ reference speed) - wRn....or.....Controller input x_{n+2}
double Rerror_p = 0; // Controller input x_{n+1}
double Rerror_pp = 0; // Controller input x_{n}

int PWMR; // Controller output for right motor
int PWML; // Controller output for left motor

double kp ;          // Controller gain kp of K = (kp + ki/s) * (100/(s+100))
double ki ;          // Controller gain ki 
double kpl ; double kpr;         // Controller gain kp of K = (kp + ki/s) * (100/(s+100))
double kil ; double kir
;         // Controller gain ki 
double alpha = 200;  // Roll off parameter alpha 
double h ;    //  prefilter parameter z = ki/kp obtained from K = (g(s+z)/s)*(100/(s+100)) 
double b1; double b0; double c1; double c0; double A;

int emergency = 1; //emergency stop feature is programmed in the robot 3 module in case of high current


// Subscriber call back to /cmd_vel
void twist_message_cmd(const geometry_msgs::Twist& msg)
{
  vd = msg.linear.x  ;
  wd = msg.angular.x ;
  vdf = msg.linear.y  ;
  wdf = msg.angular.y ;
  kp = msg.linear.z;
  ki = msg.angular.z;
  //kil = kir;
  //kpl = kpr;
  h = ki/kp;
  b0 = kpr;
  b1 = kir;
  c1 = b1 + td*(b0)/2;
  c0 =-b1 + td*(b0)/2;
  A = b0/(c1+c0);
}

// for emergency stop in case of high current
void callBack(const std_msgs::Int8& e)
{
  emergency = e.data; 
}

// Node handle
ros::NodeHandle arduino_nh ;

//geometry_msgs::Twist msg ;
//geometry_msgs::Vector3Stamped rpm_msg;
geometry_msgs::Twist rpm_msg ;


// Publisher of the right and left wheel angular velocities
//ros::Publisher pub("robot_2/arduino_vel", &rpm_msg); // Add robot_*
ros::Publisher pub("arduino_vel", &rpm_msg);

// Subscriber of the reference velocities coming from the outerloop
ros::Subscriber<geometry_msgs::Twist> sub("cmd_vel", &twist_message_cmd );
ros::Subscriber<std_msgs::Int8> sub2("emergency_stop", &callBack );


DualVNH5019MotorShield md;
void setup() {
// put your setup code here, to run once:
Serial.begin(115200);
Serial.println("Dual VNH5019 Motor Shield");
md.init();

  // Arduino node
  arduino_nh.initNode() ;

  //broadcaster.init(arduino_nh) ; //added
  
  arduino_nh.getHardware()->setBaud(115200);
  arduino_nh.advertise(pub); // setting up subscriptions
  arduino_nh.subscribe(sub); // setting up publications
  delay(1000);

}

void loop() {
 
  if (millis() - Time > sample_time)
    { 
      Time = millis() ;

      // Update Motors with corresponding speed and send speed values through serial port
     publish_data();   
     Update_Motors(vd,wd);
 

     arduino_nh.spinOnce() ;
      
    }   

  
/*  md.setM1Speed(0);
  md.setM2Speed(0);
  delay(2000); 
  md.setM1Speed(-300);
  md.setM2Speed(-300);
  delay(800);
  md.setM1Speed(0);
  md.setM2Speed(0);
  delay(2000); 
  md.setM1Speed(-200);
  md.setM2Speed(-200);
  delay(1000);
  md.setM1Speed(0);
  md.setM2Speed(0);
  delay(2000); 
  md.setM1Speed(-100);
  md.setM2Speed(-100);
  delay(1000);
  md.setM1Speed(0);
  md.setM2Speed(0);
  delay(3000); */
  // put your main code here, to run repeatedly:
//Serial.println(right_ticks);
//Serial.println(left_ticks);
}

void publish_data(){  // currently not being used 


  
  rpm_msg.linear.x = CR;//left_ticks;
  rpm_msg.linear.y = CL;
  rpm_msg.linear.z = 0;
  rpm_msg.angular.x = 0;
  rpm_msg.angular.y = wRn;//md.getM2CurrentMilliamps();
  rpm_msg.angular.z = wLn;//md.getM1CurrentMilliamps();
  pub.publish(&rpm_msg);
  //Serial.println(Time);*/

}

// UPDATE MOTORS
void Update_Motors(double vd, double wd)
{ 
  // Desired angular speed of two motors
  wdr = (2*vd + Length*wd)/(2*Radius) ; // 2*vd - wd*L
  wdl = (2*vd - Length*wd)/(2*Radius) ; // 2*vd + wd*L

  //Prefilter
  wrf = ( (td*h)*wdr + (td*h)*wdr_p - (td*h - 2)*wrf_p )/(2 + td*h);
  wlf = ( (td*h)*wdl + (td*h)*wdl_p - (td*h - 2)*wlf_p )/(2 + td*h);
  wrf_p = wrf;
  wlf_p = wlf;
  wdr_p = wdr;
  wdl_p = wdl;

  

  // Present angular velocities
  wR = (2*vdf + Length*wdf)/(2*Radius); 
  wL = (2*vdf - Length*wdf)/(2*Radius); // rads/sec

  wLn = (wL + wLp)/2.0;
  wRn = (wR + wRp)/2.0;
   
  wLp = wL; // saving present angular velocities to be used in the next loop
  wRp  = wR; // saving present angular velocities to be used in the next loop

  // I saw in the trial runs that if I don't use prefilter, the movement is very jerky !! So always use prefilter.    
  Rerror = wdr - wRn ; // error (ref - present)
  Lerror = wdl - wLn ; // error (ref - present)
  if (abs(Rerror) < 0.05)
    Rerror = 0;
  if (abs(Lerror) < 0.05)
    Lerror = 0;
  // Inner loop controller
CL = ((alpha*td*td*ki+2*alpha*td*kp)*Lerror + (2*alpha*td*td*ki)*Lerror_p + (alpha*td*td*ki-2*alpha*td*kp)*Lerror_pp + 8*CL_p - (4-2*alpha*td)*CL_pp)/(2*alpha*td + 4);
CR = ((alpha*td*td*ki+2*alpha*td*kp)*Rerror + (2*alpha*td*td*ki)*Rerror_p + (alpha*td*td*ki-2*alpha*td*kp)*Rerror_pp + 8*CR_p - (4-2*alpha*td)*CR_pp)/(2*alpha*td + 4);
// PI controller  
//  CL = (CL_p + A*c1*Lerror + A*c0*Lerror_p);  
//  CR = (CR_p + A*c1*Rerror + A*c0*Rerror_p);

  CR_p = CR;
  CL_p = CL;
  CR_pp = CR_p;
  CL_pp = CL_p;
  
  Lerror_pp = Lerror_p;
  Lerror_p = Lerror;
  Rerror_pp = Rerror_p;
  Rerror_p = Rerror; 

  PWMR = CR ;//int(255.0*CR/5.15);  // CHANGE THIS !!
  PWML = CL ;//int(255.0*CL/5.15);  // CHANGE THIS !!

  // Saturating input commands to right motor   
 if (PWMR>=300) 
  {  
    PWMR=300;
  } 
  else if (PWMR<=-1) 
  {
    PWMR=0 ;
  }
  
  // Saturating input commands to left motor
  if (PWML>=300) 
  {
    PWML=300 ;
  }
  else if (PWML<=-1) 
  {
    PWML=0 ;
  }   

  CL_p = PWML;
  CR_p = PWMR;
  // Running the motors
  md.setM1Speed(-PWMR*emergency) ; // PWML 
  md.setM2Speed(-PWML*emergency) ; // PWMR
  //md.setM1Speed(100) ; // l  +ve(YES)/-ve(NO) PWML
  //md.setM2Speed(100) ;    
  
}
