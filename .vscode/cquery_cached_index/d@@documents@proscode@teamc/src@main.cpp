#include "main.h"
#include "display/lv_themes/lv_theme_night.h"
#include "display/lv_themes/lv_theme_night.c"
//these are the variables that will hold the speed of a
//motor and print it for auton recorder
int LeftDriveFrontSpeed;	            //first motor
int LeftDriveBackSpeed;          //third motor
int LeftIntakeSpeed;           //second motor
int RightDriveFrontSpeed;        //fourth motor
int RightDriveBackSpeed;		      //sixth motor
int RightIntakeSpeed;         //fifth motor
int DropperSpeed;               //seventh motor
int FourBarSpeed;

//thee are the 3 variables of time used in the
//auton recorder
int timeOld;
int timeNew;
int deltaTime;

///////////////////initialize all sensors here/////////////////////////////////
ADIUltrasonic backSonar('E', 'F');						//sonar sensor
ADIButton LeftDriveButton('G');								//buttons under the base
ADIButton RightDriveButton('H');							//buttons under the base


Controller controller;               // Joystick to read analog values for tank or arcade control.

ControllerButton rightUp = controller[ControllerDigital::R1];
ControllerButton rightDown = controller[ControllerDigital::R2];
ControllerButton leftUp = controller[ControllerDigital::L1];
ControllerButton leftDown = controller[ControllerDigital::L2];

ControllerButton btnDown = controller[ControllerDigital::down];
ControllerButton btnUp = controller[ControllerDigital::up];
ControllerButton btnRight = controller[ControllerDigital::right];
ControllerButton btnLeft = controller[ControllerDigital::left];
ControllerButton btnA = controller[ControllerDigital::A];
ControllerButton btnB = controller[ControllerDigital::B];
ControllerButton btnX = controller[ControllerDigital::X];
ControllerButton btnY = controller[ControllerDigital::Y];

float leftY = controller.getAnalog(ControllerAnalog::leftY);
float leftX = controller.getAnalog(ControllerAnalog::leftX);
float rightY = controller.getAnalog(ControllerAnalog::rightY);
float rightX = controller.getAnalog(ControllerAnalog::rightX);

MotorGroup left = {LeftDriveFront, LeftDriveBack};
MotorGroup right = {RightDriveFront, RightDriveBack};

auto drive =  ChassisControllerFactory::create(
	left, right,
	AbstractMotor::gearset::green,
	{2.75_in, 10.5_in}
);


bool bringDropperBack = false;
bool getReadyToDrop = false;
bool bringLiftBack = false;
bool hasTrayDropped = false;
bool isRecording = false;

float dropperMax = 1300;
float dropperError;
int dropperTargetSpeed;
int d_N = 90;
float d_d = 5;
int d_m = 10;
int dropperGetReadyPos = 980;
float e = 2.71828;

float intakeMultiplier = 45;
static void text(int l, std::string message);

void intake(int speed){
 LeftIntake.moveVelocity(speed);
 RightIntake.moveVelocity(speed);
}
void intakeDist(int dist, int speed){
 LeftIntake.moveAbsolute(dist, speed);
 RightIntake.moveAbsolute(dist, speed);
}


/*****************************************************LV stuff**************************************************************/
const char * motorMapLayout[] = {"\223FL", "\242", "\223FR", "\242", "\n",
"\223BL", "\242", "\223BR", "\242", "\n",
"\223Left In", "\242", "\223Right In", "\242", "\n",
"\223Dropper", "\242", "\2234Bar", "\242", ""};
static lv_obj_t * scr;
static lv_style_t abar_indic;
lv_obj_t * textContainter;


lv_obj_t * lines[10];

void text(int l, std::string message){
	const char* c = message.c_str();
	lv_label_set_text(lines[l], c);
}
extern lv_theme_t * lv_theme_night_init(uint16_t hue, lv_font_t *font);


std::string motorNameString;
static lv_res_t btnm_action(lv_obj_t * btnm, const char * txt){
	if((std::string)(txt) == "FL"){
		DisplayMotor = {LeftDriveFront};
	}else if((std::string)(txt) == "BL"){
		DisplayMotor = {LeftDriveBack};
	}else if((std::string)(txt) == "FR"){
		DisplayMotor = {RightDriveFront};
	}else if((std::string)(txt) == "BR"){
		DisplayMotor = {RightDriveBack};
	}else if((std::string)(txt) == "Left In"){
		DisplayMotor = {LeftIntake};
	}else if((std::string)(txt) == "Right In"){
		DisplayMotor = {RightIntake};
	}else if((std::string)(txt) == "Dropper"){
		DisplayMotor = {Dropper};
	}else if((std::string)(txt) == "4Bar"){
		DisplayMotor = {FourBar};
	}
	motorNameString = (std::string)txt;
	return LV_RES_OK;
}
int batteryCharge;


/*****************************************************OPCONTROL***********************************************************/
//285 = 90 degy

lv_obj_t * hubTab;
lv_obj_t * textTab;
lv_obj_t * visionTab;

void resetDrive(){
	left.tarePosition();
	right.tarePosition();
}

void leftTime(int dist){
	resetDrive();
	while(abs(left.getPosition())<abs(dist)-3){
		pros::delay(10);
	}
}

void rightTime(int dist){
	resetDrive();
	while(abs(right.getPosition())<abs(dist)){
		pros::delay(10);
	}
}
/**
 * A callback function for LLEMU's center button.
 *
 * When this callback is fired, it will toggle line 2 of the LCD text between
 * "I was pressed!" and nothing.
 */
 //array of autons
 std::string arr[] = {"red vertical (0)", "red horizontal (1)", "blue vertical (2)", "blue horizontal (3)", "Preload(4)", "blue skills (5)"};
 int autNum = 0; //selected auton
 void on_center_button() {
 		pros::lcd::set_text(0, arr[autNum]); //does nothing

 }

 void on_right_button(){ //increment autNum to next auton
 		 autNum = autNum + 1;
 		 autNum = autNum%6;
 		 pros::lcd::set_text(0, arr[autNum]);
 }

 void on_left_button(){ //decrement autNum and check for negative
 	autNum = autNum - 1;
 	if(autNum < 0){
 		autNum = autNum + 6;
 	}
 	autNum = autNum%6;
 	pros::lcd::set_text(0, arr[autNum]);
 }

/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */
void initialize() {
	pros::lcd::initialize();
		pros::lcd::register_btn0_cb(on_left_button);
		pros::lcd::register_btn1_cb(on_center_button);
		pros::lcd::register_btn2_cb(on_right_button);
}

/**
 * Runs while the robot is in the disabled state of Field Management System or
 * the VEX Competition Switch, following either autonomous or opcontrol. When
 * the robot is enabled, this task will exit.
 */
void disabled() {}

/**
 * Runs after initialize(), and before autonomous when connected to the Field
 * Management System or the VEX Competition Switch. This is intended for
 * competition-specific initialization routines, su ch as an autonomous selector
 * on the LCD.
 *
 * This task will exit when the robot is enabled and autonomous or opcontrol
 * starts.
 */
void competition_initialize() {}

/**
 * Runs the user autonomous code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the autonomous
 * mode. Alternatively, this function may be called in initialize or opcontrol
 * for non-competition testing purposes.
 *
 * If the robot is disabled or communications is lost, the autonomous task
 * will be stopped. Re-enabling the robot will restart the task, not re-start it
 * from where it left off.
 */
 void bluUnprotected(){
	 drive.setBrakeMode(AbstractMotor::brakeMode(0));
	 intake(-200);
	 FourBar.moveAbsolute(-1200, 200);
	 pros::delay(350);
	 intake(0);
	 pros::delay(1750);
	 FourBar.moveAbsolute(15, 200);
	 pros::delay(1100);

	 intake(200);

	 drive.setMaxVelocity(70);
	 drive.moveDistance(1200);
	 pros::delay(700);
	 intake(20);
	 Dropper.moveAbsolute(1500, 200);
	 drive.moveDistance(-950);

drive.turnAngle(-240);
bool leftCheck = false;
bool rightCheck = false;
while(!leftCheck || !rightCheck){
	if(!LeftDriveButton.isPressed() && !leftCheck){
		left.moveVelocity(150);
	}else{
			leftCheck = false;
			left.moveVelocity(0);
	}
	if(!RightDriveButton.isPressed() && !rightCheck){
		right.moveVelocity(160);
	}else{
			right.moveVelocity(0);
			rightCheck = true;
	}

}
Dropper.moveAbsolute(1770, 120);
pros::delay(1200);

intake(-30);
drive.setMaxVelocity(-70);
drive.moveDistance(-200);

 }
void redUnprotected(){
	drive.setBrakeMode(AbstractMotor::brakeMode(0));
	intake(-200);
	FourBar.moveAbsolute(-1200, 200);
	pros::delay(350);
	intake(0);
	pros::delay(1750);
	FourBar.moveAbsolute(15, 200);
	pros::delay(1100);

	intake(200);

	drive.setMaxVelocity(70);
	drive.moveDistance(1200);
	pros::delay(700);
	intake(20);
	Dropper.moveAbsolute(1500, 200);
	drive.moveDistance(-950);

drive.turnAngle(240);
bool leftCheck = false;
bool rightCheck = false;
while(!leftCheck || !rightCheck){
 if(!LeftDriveButton.isPressed() && !leftCheck){
	 left.moveVelocity(160);
 }else{
		 leftCheck = false;
		 left.moveVelocity(0);
 }
 if(!RightDriveButton.isPressed() && !rightCheck){
	 right.moveVelocity(150);
 }else{
		 right.moveVelocity(0);
		 rightCheck = true;
 }

}
Dropper.moveAbsolute(1770, 120);
pros::delay(1200);

intake(-30);
drive.setMaxVelocity(-70);
drive.moveDistance(-200);

}
void autonomous() {
redUnprotected();
}

/**
 * Runs the operator control code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the operator
 * control mode.
 *
 * If no competition control is connected, this function will run immediately
 * following initialize().
 *
 * If the robot is disabled or communications is lost, the
 * operator control task will be stopped. Re-enabling the robot will restart the
 * task, not resume it from where it left off.
 */


void opcontrol() {
	drive.setBrakeMode(AbstractMotor::brakeMode(0));
	FourBar.setBrakeMode(AbstractMotor::brakeMode::hold);
	LeftIntake.setBrakeMode(AbstractMotor::brakeMode::hold);
	RightIntake.setBrakeMode(AbstractMotor::brakeMode::hold);
	Dropper.setBrakeMode(AbstractMotor::brakeMode::hold);
	int shakeIntake = 1;

	// set up the screen
   lv_theme_set_current(lv_theme_night_init(0, NULL));
   scr = lv_page_create(NULL, NULL);
   lv_obj_set_size(scr, 480, 272);
   lv_scr_load(scr);

   lv_obj_t * tabview = lv_tabview_create(lv_scr_act(), NULL);
   hubTab = lv_tabview_add_tab(tabview, "Hub");
   textTab = lv_tabview_add_tab(tabview, "Text");
   visionTab = lv_tabview_add_tab(tabview, "Vision");


   //  lv_obj_set_size(tabview, 10, 10);


   lv_obj_t * motorButtons = lv_btnm_create(hubTab, NULL);
   lv_btnm_set_map(motorButtons, motorMapLayout);
   lv_obj_set_pos(motorButtons, 0, 0);
   lv_obj_set_size(motorButtons, 170, 130);
   lv_btnm_set_action(motorButtons, btnm_action);
   lv_obj_t * motorContainer = lv_cont_create(hubTab, NULL);
   lv_obj_set_pos(motorContainer, 200, 55);
   lv_obj_set_size(motorContainer, 200, 100);
   lv_obj_t * motorName = lv_label_create(motorContainer, NULL);
   lv_obj_set_pos(motorName, 0, 5);
   lv_obj_t * motorTemp = lv_label_create(motorContainer, NULL);
   lv_obj_align(motorTemp, motorName, LV_ALIGN_OUT_BOTTOM_MID, 5, 3);
   lv_obj_t * motorPosition = lv_label_create(motorContainer, NULL);
   lv_obj_align(motorPosition, motorTemp, LV_ALIGN_OUT_BOTTOM_MID, 5, 3);

   lv_obj_t * batteryBar = lv_bar_create(hubTab, NULL);
   lv_obj_set_size(batteryBar, 220, 20);
   lv_obj_set_pos(batteryBar, 185, -20);
   lv_bar_set_range(batteryBar, 0, 100);
   lv_obj_align(batteryBar, motorButtons, LV_ALIGN_OUT_TOP_RIGHT, 50, 0);
   lv_obj_t * batteryLabel = lv_label_create(hubTab, NULL);
   lv_obj_align(batteryLabel, batteryBar, LV_ALIGN_IN_RIGHT_MID, 50, 0);
   lv_label_set_text(batteryLabel, ("" + std::to_string(pros::battery::get_capacity())+ "%").c_str() );
   lv_obj_set_size(batteryLabel, 240, 15);
   lv_obj_t * TLbar = lv_bar_create(motorButtons, NULL);
   lv_bar_set_range(TLbar, 25, 60);
   lv_obj_set_size(TLbar, 10, 25);
   lv_obj_set_pos(TLbar, 60, 3);
   lv_obj_t * TRbar = lv_bar_create(motorButtons, TLbar);
   lv_obj_align(TRbar, TLbar, LV_ALIGN_OUT_RIGHT_MID, 76, 0);
   lv_obj_t * CLbar = lv_bar_create(motorButtons, TLbar);
   lv_obj_align(CLbar, TLbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
   lv_obj_t * BLbar = lv_bar_create(motorButtons, TLbar);
   lv_obj_align(BLbar, CLbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
   lv_obj_t * Catabar = lv_bar_create(motorButtons, TLbar);

   lv_obj_align(Catabar, BLbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
   lv_obj_t * CRbar = lv_bar_create(motorButtons, TLbar);
   lv_obj_align(CRbar, TRbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
   lv_obj_t * BRbar = lv_bar_create(motorButtons, TLbar);
   lv_obj_align(BRbar, CRbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
   lv_obj_t * Intakebar = lv_bar_create(motorButtons, TLbar);
   lv_obj_align(Intakebar, BRbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);


   textContainter = lv_cont_create(textTab, NULL);
   lv_obj_align(textContainter, textTab, LV_ALIGN_IN_TOP_LEFT, 0, 0);
   lv_obj_set_size(textContainter, 480, 240);

   for(int i = 0; i < 10; i++){
     lines[i] = lv_label_create(textContainter, NULL);
     lv_obj_set_x(lines[i], 15);
     lv_obj_set_y(lines[i], 20*i);
     lv_label_set_text(lines[i], "");
   }

	while(true){
		//lvgl stuff///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		     batteryCharge = (int)(pros::battery::get_capacity());
		     lv_label_set_text(batteryLabel, ("" + std::to_string(pros::battery::get_capacity())+ "%").c_str() );
		     lv_bar_set_value(batteryBar, batteryCharge);
		     lv_bar_set_value(TLbar, LeftDriveBack.getTemperature());
		     lv_bar_set_value(TRbar, RightDriveBack.getTemperature());
		     lv_bar_set_value(CLbar, LeftIntake.getTemperature());
		     lv_bar_set_value(CRbar, RightIntake.getTemperature());
		     lv_bar_set_value(BLbar, LeftDriveFront.getTemperature());
		     lv_bar_set_value(BRbar, LeftDriveBack.getTemperature());
		     lv_bar_set_value(Catabar, FourBar.getTemperature());
		     lv_bar_set_value(Intakebar, Dropper.getTemperature());

		     lv_label_set_text(motorName, ("Motor --- " + motorNameString).c_str());
		     lv_label_set_text(motorTemp, ("Temp : " + std::to_string(DisplayMotor.getTemperature())).c_str());
		     lv_label_set_text(motorPosition, ("Pos : " + std::to_string(DisplayMotor.getPosition())).c_str());

		//drive code///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		leftY = controller.getAnalog(ControllerAnalog::leftY);
		leftX = controller.getAnalog(ControllerAnalog::leftX);
		rightY = controller.getAnalog(ControllerAnalog::rightY);
		rightX = controller.getAnalog(ControllerAnalog::rightX);


		if(rightUp.isPressed() && rightDown.isPressed()){

		}else if(!btnA.isPressed()){
			if(btnUp.changedToPressed() && !getReadyToDrop){					//4-Bar up
			LeftIntake.tarePosition();
			RightIntake.tarePosition();
			timeOld = pros::millis();
			timeNew = pros::millis();
			deltaTime = timeNew - timeOld;
			 while(FourBar.getPosition()>-1225 && deltaTime<5000){
				 leftY = controller.getAnalog(ControllerAnalog::leftY);
				 leftX = controller.getAnalog(ControllerAnalog::leftX);
				 rightY = controller.getAnalog(ControllerAnalog::rightY);
				 rightX = controller.getAnalog(ControllerAnalog::rightX);
				 FourBar.moveVelocity(-200);
				 drive.arcade(leftY, rightX, .07); //Normal Drive code
				 pros::delay(50);
				 deltaTime = timeNew - timeOld;
			 }
			 FourBar.moveVelocity(0);

		 }else if(btnDown.changedToPressed()){
			 leftY = controller.getAnalog(ControllerAnalog::leftY);
			 leftX = controller.getAnalog(ControllerAnalog::leftX);
			 rightY = controller.getAnalog(ControllerAnalog::rightY);
			 rightX = controller.getAnalog(ControllerAnalog::rightX);
			 timeOld = pros::millis();
			 timeNew = pros::millis();
			 deltaTime = timeNew - timeOld;
			 if(FourBar.getPosition()>-950){
				while(FourBar.getPosition()>-960 && deltaTime<5000){
					FourBar.moveVelocity(-200);
					drive.arcade(leftY, rightX, .07); //Normal Drive code
					pros::delay(50);
					deltaTime = timeNew - timeOld;
					pros::delay(50);
				}
				FourBar.moveVelocity(0);
			}else{
				while(FourBar.getPosition()<-960 && deltaTime<5000){
					leftY = controller.getAnalog(ControllerAnalog::leftY);
					leftX = controller.getAnalog(ControllerAnalog::leftX);
					rightY = controller.getAnalog(ControllerAnalog::rightY);
					rightX = controller.getAnalog(ControllerAnalog::rightX);
					FourBar.moveVelocity(200);
					drive.arcade(leftY, rightX, .07); //Normal Drive code
					pros::delay(50);
					deltaTime = timeNew - timeOld;
					pros::delay(50);
				}
				FourBar.moveVelocity(0);
			}
		}else if(rightUp.isPressed()){
	 		 FourBar.moveVelocity(-150);
	 	 }else if(rightDown.isPressed()){
	 		 FourBar.moveVelocity(150);
	 	 }else{
	 		 FourBar.moveVelocity(0);
	 	 }
	 }

	 if(leftUp.isPressed() && leftDown.isPressed()){
			 if(FourBar.getPosition() < -1000){
				 LeftIntake.moveVelocity(-200);
				 RightIntake.moveVelocity(-200);
			 }else if(FourBar.getPosition() < -800){
				 LeftIntake.moveVelocity(-150);
				 RightIntake.moveVelocity(-150);
			 }else{
				 LeftIntake.moveVelocity(200);
				 RightIntake.moveVelocity(200);
			 }

			 drive.arcade(leftY, rightX, .07); //Normal Drive code
	 }else	if(btnB.isPressed()){//used to keep the cube in intake stationary
			drive.arcade(leftY/3.5, rightX/3, 0.05);				//drive moves back
			LeftIntake.moveVelocity(leftY*70);						//intake moves out
			RightIntake.moveVelocity(leftY*70);					//2 motions cancel out and cube does not move

	 }else{
		 if(leftUp.isPressed()){					//Intake in
			 LeftIntake.moveVelocity(100);
			 RightIntake.moveVelocity(100);
		 }else if(leftDown.isPressed()){	//Intake out
			 LeftIntake.moveVelocity(-120);
			 RightIntake.moveVelocity(-120);
		 }else if(btnLeft.isPressed()){	//rotate cube
			 LeftIntake.moveVelocity(-120);
			 RightIntake.moveVelocity(120);
		 }else if(rightUp.isPressed() && rightDown.isPressed()){
			 intake(-30);
		 }else{														//Intake off
			 LeftIntake.moveVelocity(0);
			 RightIntake.moveVelocity(0);
		 }
		 drive.arcade(leftY, rightX, .07); //Normal Drive code
	 }
	 if(btnA.changed()&&Dropper.getPosition()<1760){
		 hasTrayDropped = false;
	 }
	 if(btnA.isPressed()){							//ENTER DROPPER MODE
		 if(rightUp.isPressed()){					//Dropper up
			 try{
			 // dropperTargetSpeed = (int)((1.0/(1.0 + d_m*std::pow(e,((-1.0*dropperError)/d_d))))*(200-d_N)+d_N);
			 dropperTargetSpeed = (Dropper.getPosition()<1300? 200: (Dropper.getPosition()<1550? 120: 50));
			 }catch(std::exception e){
				 text(2, e.what());
			 dropperTargetSpeed=10;
			 }
			 // Dropper.moveVelocity((((dropperMax-Dropper.getPosition())/dropperMax)*200)>50?(((3000-Dropper.getPosition())/2800)*200):50);
			 Dropper.moveVelocity(dropperTargetSpeed);
		 }else if(rightDown.isPressed()){//Dropper down
			 Dropper.moveVelocity(-180);
		 }else if(btnY.isPressed()){
			Dropper.moveVelocity(-200);
		}else{
			 Dropper.moveVelocity(0);		//Dropper off
		 }


		 if(leftUp.isPressed()){					//Intake in slowly
			 LeftIntake.moveVelocity(30);
			 RightIntake.moveVelocity(30);
		 }else if(leftDown.isPressed()){	//Intake out slowly
			 LeftIntake.moveVelocity(-30);
			 RightIntake.moveVelocity(-30);
		 }else{													//Intake off
			 LeftIntake.moveVelocity(leftY<0?leftY*intakeMultiplier:0);
			 RightIntake.moveVelocity(leftY<0?leftY*intakeMultiplier:0);
		 }
		 drive.arcade(leftY/3.5,leftX/5);//slow drive
		 FourBar.moveVelocity(0);				//lift off
	 }else{
		 Dropper.moveVoltage(0);
	 }																	//EXIT DROPPER mode

	 if(btnB.changedToPressed()){
		 if(Dropper.getPosition() > 400){
			 bringDropperBack = true;
		 }else if(Dropper.getPosition() <= 400 && FourBar.getPosition()>-50){
			 getReadyToDrop = true;
		 }
		 if(FourBar.getPosition() < -2){
			 bringLiftBack = true;
		 }
	 }

	 if(bringDropperBack){
		 if(Dropper.getPosition() > 2){
			 Dropper.moveVelocity(-200);
			 intake(40);
		 }else{
			 Dropper.moveVelocity(0);
			 intake(0);
			 bringDropperBack = false;
		 }
	 }
	 if(getReadyToDrop){
		 if(Dropper.getPosition()<dropperGetReadyPos){
			 Dropper.moveVelocity(200);
			 intake(200);
		 }else{
			 Dropper.moveVelocity(0);
			 intake(0);
			 getReadyToDrop = false;
		 }
	 }
	 if(bringLiftBack){
		 if(FourBar.getPosition() < -50){
			 FourBar.moveVelocity(200);
		 }else{
			 FourBar.moveVelocity(0);
			 bringLiftBack = false;
		 }
	 }

	 text(0, std::to_string(backSonar.get()));
	 text(1, std::to_string(LeftDriveButton.isPressed()));
	 text(2, std::to_string(RightDriveButton.isPressed()));


		pros::delay(50);

		if(isRecording){
			LeftDriveBackSpeed = LeftDriveBack.getTargetVelocity();
			LeftDriveFrontSpeed = LeftDriveFront.getTargetVelocity();
			RightDriveBackSpeed = RightDriveBack.getTargetVelocity();
			RightDriveFrontSpeed = RightDriveFront.getTargetVelocity();
			LeftIntakeSpeed = LeftIntake.getTargetVelocity();
			RightIntakeSpeed = RightIntake.getTargetVelocity();
			FourBarSpeed = FourBar.getTargetVelocity();
			DropperSpeed = Dropper.getTargetVelocity();

			FILE* USDWriter = fopen("/usd/run1.txt", "a");

			fprintf(USDWriter, "LeftDriveBack.moveVelocity(%i); \n", LeftDriveBackSpeed);
			fprintf(USDWriter, "LeftDriveFront.moveVelocity(%i); \n", LeftDriveFrontSpeed);
			fprintf(USDWriter, "RightDriveBack.moveVelocity(%i); \n", RightDriveBackSpeed);
			fprintf(USDWriter, "RightDriveFront.moveVelocity(%i); \n", RightDriveFrontSpeed);
			fprintf(USDWriter, "LeftIntake.moveVelocity(%i); \n", LeftIntakeSpeed);
			fprintf(USDWriter, "RightIntake.moveVelocity(%i); \n", RightIntakeSpeed);
			fprintf(USDWriter, "FourBar.moveVelocity(%i); \n", FourBarSpeed);
			fprintf(USDWriter, "DropperSpeed.moveVelocity(%i); \n", DropperSpeed);

			timeNew = pros::millis();
			deltaTime = timeNew - timeOld;
			timeOld = pros::millis();

			fprintf(USDWriter, "pros::delay(%i); \n", deltaTime);
			fclose(USDWriter);


		}//auton recorder
	}//while loop
}//op control
