#ifdef _M_IX86
#include <windows.h>
#else
#include <stream.h>
#endif

#include <string.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>
#include <math.h>
#include <time.h>

#define		MAXROID 24					// Max number of asteroids
#define		MAXLARGEROID 6				// Max number of Large asteroids
#define		MAXRAND 6.6					// Max x or y value for translation
#define		MINRAND -6.6				// Min x or y value for translation
#define		VMAXRAND .1					// Max x or y value for translation speed
#define		VMINRAND -.1				// Min x or y value for translation speed
#define		PLAYER_ROT 8				// Speed of player rotation
#define		SPEED .005
#define		MAX_SPEED  .3				// Max speed for player ship
#define		M_PI 3.14159265358979323846 // Value of Pi
#define		MAX_SHOTS 4					// Max number of shots
#define		SHOT_SIZE 8.0f				// Size of shot
#define		PART_SIZE 4.0f				// Size of shot
#define		PART_SPEED 0.2f				// Speed of particle
#define		SHOT_SPEED .3				// Shot Speed
#define		SHOT_LIFETIME 25			// Max lifetime for shots
#define		PART_LIFETIME 10			// Max lifetime for shots
#define		MAX_PART 96					// Max number of particles
#define		LARGE_ASTEROID_SCORE 20		// Number of points for scored shooting a large asteroid
#define		MEDIUM_ROID_SCORE 50		// Number of points scored for shooting a medium asteroid
#define		SMALL_ROID_SCORE 100		// Number of points scored for shooting a small asteroid
#define		LARGE_ALIEN_SCORE 200		// Number of points scored for shooting a large alien ship
#define		SMALL_ALIEN_SCORE 1000		// Number of points scored for shooting a small alien ship
#define		ALIEN_SHIP_LARGE_SPEED .05		// Speed of alien ship ship horizontally when large
#define		ALIEN_SHIP_SMALL_SPEED .1		// Speed of alien ship ship horizontally when small
#define		ALIEN_SHIP_ROTATION 1.0F	// Rotation speed of alien ship
#define		ALIEN_SHIP_MOVE_FREQ 25		// Frequency of switching y directions for alien ship
#define		MINSCORE 2000				// Minimum score required to get small aliens
#define		BONUS_SHIP_SCORE 5000		// Score to get new ship
#define		ALIEN_TIMER 200				// Time until alien can spawn

HDC			hDC = NULL;                 // Private GDI Device Context
HGLRC       hRC = NULL;                 // Permanent Rendering Context
HWND        hWnd = NULL;                // Holds Our Window Handle
HINSTANCE   hInstance;                  // Holds The Instance Of The Application

bool        keys[256];                  // Array Used For The Keyboard Routine
bool        active = TRUE;              // Window Active Flag
bool        fullscreen = TRUE;          // Fullscreen Flag Set To TRUE By Default

bool		isAlive = false;			// is player alive?

int			gameDelay = 33;				// delay in msec	

int			numRoid = 0;				// Current number of asteroids
int			bullets = 0;				// Current number of bullet

int			numPart = 0;				// Current number of explosion particles

bool		canShoot = true;			// Can player shoot
bool		canRotate = false;			// Can player rotate
bool		canAccelerate = false;
bool		rotateCW = false;			// Can player rotate clockwise
int			playerLives = 4;			// Number of player lives

bool		levelComplete = true;		// Does the next level need to be generated?
int			roidToGenerate = 1;			// How many asteroids should we spawn?

int			playerScore = 0;			// Current score for player
int			scoreSinceLastBonus = 0;
int			timeSinceLastAlien = 0;		// Current player lifetime in current screen
bool		hasLrgAlienSpawned = false;	// Has Large Alien Spawned in current screen

int			testArray[1] = { playerLives };

//--------------------------------------------
// Explosion class
//--------------------------------------------

class Explosion {
public:
	GLfloat x;
	GLfloat y;
	GLfloat vx;
	GLfloat vy;
	int lifetime;
	void display();
	void setup(GLfloat, GLfloat, GLfloat, GLfloat);
	void move();
	void age();
}particles[MAX_PART];

void Explosion::display() {
	glPushMatrix();
	glTranslatef(x, y, 0);				  // Translate particle
	glColor3f(1.0f, 1.0f, 0.0f);          // Set The Color To Green
	glPointSize(PART_SIZE);				  // Make particles smaller
	glBegin(GL_POINTS);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glEnd();
	glPopMatrix();
}

void Explosion::setup(GLfloat newX, GLfloat newY, GLfloat newVX, GLfloat newVY) {
	x = newX;
	y = newY;
	vx = newVX;
	vy = newVY;
	lifetime = 0;
}

void Explosion::move() {
	x += vx;
	y += vy;
}

void Explosion::age() {
	lifetime++;
}

//--------------------------------------------
// Shot class
//--------------------------------------------

class Shot {

public:
	GLfloat		tShotx;		// x postion of the shot
	GLfloat		tShoty;		// y position the shot
	GLfloat		vShotx;		// velocity for moving the shot in the x
	GLfloat		vShoty;		// velocity for moving the shot in the y
	bool		destroyed;	// has the shot dissiapted from hitting an asteroid
	int			lifetime;	// current time alive
	bool		alienShot = false;		// Lethal to player
	void display_shot();
	void position_shot(GLfloat, GLfloat, GLfloat, GLfloat, GLfloat, bool);
	void move_shot();
	int age();
}shot[MAX_SHOTS + 1];

void Shot::display_shot() {
	glPushMatrix();
	glTranslatef(tShotx, tShoty, 0);	  // Translate shot
	if (alienShot) {
		glColor3f(1.0f, 0.0f, 0.0f);	// Set The Color To Red
		glPointSize(SHOT_SIZE/2);				  // Make shots larger
	}
	else {
		glColor3f(0.0f, 1.0f, 0.0f);      // Set The Color To Green
		glPointSize(SHOT_SIZE);				  // Make shots larger
	}
	// Draw Shot
	glBegin(GL_POINTS);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glEnd();
	glPopMatrix();
}

void Shot::position_shot(GLfloat x, GLfloat y, GLfloat vx, GLfloat vy, GLfloat rot, bool lethal) {
	lifetime = 0;
	tShotx = x;
	tShoty = y;
	vShotx = cos(rot * (M_PI / 180)) * SHOT_SPEED + vx;		// Move by velocity in x
	vShoty = sin(rot * (M_PI / 180)) * SHOT_SPEED + vy;		// Move by velocity in y
	destroyed = false;
	alienShot = lethal;

}

void Shot::move_shot() {
	tShotx += vShotx;
	tShoty += vShoty;
	if (tShotx > MAXRAND || tShotx < MINRAND) {
		tShotx = tShotx * -1;
	}
	if (tShoty > MAXRAND || tShoty < MINRAND) {
		tShoty = tShoty * -1;
	}
}

int Shot::age() {
	lifetime++;
	return lifetime;
}

//--------------------------------------------
// Asteroid class
//--------------------------------------------

class Asteroid {

public:
	GLfloat		rroidx;		// vector rotation For the Asteroid
	GLfloat		rroidy;		// vector rotation For the Asteroid
	GLfloat		rroidz;		// vector rotation For the Asteroid
	GLfloat		troidx;		// X movement For the Asteroid
	GLfloat		troidy;		// Y movement For the Asteroid
	GLfloat		vroidx;		// x movement speed For the Asteroid
	GLfloat		vroidy;		// y movement speed For the Asteroid
	GLfloat		rroid;		// Angle of rotation For the Asteroid
	GLfloat		vrroid;		// Angle of rotation speed For the Asteroid

	GLfloat		scaleX;		// Scale the object in the x axis
	GLfloat		scaleY;		// Scale the object in the y axis
	GLfloat		scaleZ;		// Scale the object in the z axis

	GLfloat		red;		// Value for red in RGB
	GLfloat		green;		// Value for green in RGB
	GLfloat		blue;		// Value for blue in RGB

	GLfloat		radius;		// Radius of object

	bool		isLarge;	// Is asteroid a large asteroid?
	bool		isMedium;	// Is asteroid a large asteroid?
	bool		destroyed;	// Is the object destroyed?

	void display_roid();
	void setup_roid();
	void move_roid();
	void split();
	void score();
}asteroid[MAXROID];

void Asteroid::display_roid() {
	// Draw Asteroid
	// I used an object rendering function to draw the object for HW2 to simplify drawing the object
	glPushMatrix();
	glTranslatef(troidx, troidy, 0);			// Move asteroid in a random direction
	glRotatef(rroid, rroidx, rroidy, rroidz);    // Rotate The ship on a random axis
	glScalef(scaleX, scaleY, scaleZ);					// Make it smaller
	glColor3f(red, green, blue);           // Set The Color To Blue
	glutSolidDodecahedron();
	glPopMatrix();
}

void Asteroid::setup_roid() {
	// If all rotation on all axes are 0, try again.
	while (rroidx + rroidy + rroidz == 0) {
		rroidx = rand() % 2;
		rroidy = rand() % 2;
		rroidz = rand() % 2;
	}

	int side = rand() % 2;

	vrroid = (MAXRAND - MINRAND) * ((float)rand() / (float)RAND_MAX) + MINRAND;
	vroidx = (VMAXRAND - VMINRAND) * ((double)rand() / (double)RAND_MAX) + VMINRAND;
	vroidy = (VMAXRAND - VMINRAND) * ((double)rand() / (double)RAND_MAX) + VMINRAND;

	// 0 = top/bottom
	// 1 = left/right
	if (side == 0) {
		troidx = (MAXRAND - MINRAND) * ((double)rand() / (double)RAND_MAX) + MINRAND;
		troidy = MINRAND;
	}
	else if (side == 1) {
		troidx = MINRAND;
		troidy = (MAXRAND - MINRAND) * ((double)rand() / (double)RAND_MAX) + MINRAND;
	}

	red = 0.0f;
	green = 1.0f;
	blue = 0.0f;

	scaleX = .6;
	scaleY = .6;
	scaleZ = .6;

	radius = 0.6f * sqrt(3.0f);

	isLarge = true;

	destroyed = false;
}

void Asteroid::move_roid() {
	troidx += vroidx;		// Move by velocity in x
	troidy += vroidy;		// Move by velocity in y

	rroid += vrroid;		// Rotate ateroid by random speed

	if (troidx > MAXRAND || troidx < MINRAND) {
		troidx = troidx * -1;
	}
	if (troidy > MAXRAND || troidy < MINRAND) {
		troidy = troidy * -1;
	}
}

void Asteroid::split() {

	if (isLarge) {
		scaleX = .4;
		scaleY = .4;
		scaleZ = .4;

		red = 0.0f;
		green = 0.0f;
		blue = 1.0f;

		radius = 0.4f * sqrt(3.0f);

		// If all rotation on all axes are 0, try again.
		while (rroidx + rroidy + rroidz == 0) {
			rroidx = rand() % 2;
			rroidy = rand() % 2;
			rroidz = rand() % 2;
		}

		vrroid = (MAXRAND - MINRAND) * ((float)rand() / (float)RAND_MAX) + MINRAND;
		vroidx = (VMAXRAND - VMINRAND) * ((double)rand() / (double)RAND_MAX) + VMINRAND;
		vroidy = (VMAXRAND - VMINRAND) * ((double)rand() / (double)RAND_MAX) + VMINRAND;

		isLarge = false;
		isMedium = true;

		asteroid[numRoid].setup_roid();
		asteroid[numRoid].troidx = troidx;
		asteroid[numRoid].troidy = troidy;

		asteroid[numRoid].scaleX = .4;
		asteroid[numRoid].scaleY = .4;
		asteroid[numRoid].scaleZ = .4;

		asteroid[numRoid].red = 0.0f;
		asteroid[numRoid].green = 0.0f;
		asteroid[numRoid].blue = 1.0f;

		asteroid[numRoid].radius = 0.4f * sqrt(3.0f);

		asteroid[numRoid].isLarge = false;
		asteroid[numRoid].isMedium = true;
		asteroid[numRoid].destroyed = false;

		numRoid++;
	}
	else if (isMedium) {
		scaleX = .2;
		scaleY = .2;
		scaleZ = .2;

		red = 1.0f;
		green = 1.0f;
		blue = 1.0f;

		radius = 0.2f * sqrt(3.0f);

		// If all rotation on all axes are 0, try again.
		while (rroidx + rroidy + rroidz == 0) {
			rroidx = rand() % 2;
			rroidy = rand() % 2;
			rroidz = rand() % 2;
		}

		vrroid = (MAXRAND - MINRAND) * ((float)rand() / (float)RAND_MAX) + MINRAND;
		vroidx = (VMAXRAND - VMINRAND) * ((double)rand() / (double)RAND_MAX) + VMINRAND;
		vroidy = (VMAXRAND - VMINRAND) * ((double)rand() / (double)RAND_MAX) + VMINRAND;

		isLarge = false;
		isMedium = false;;

		asteroid[numRoid].setup_roid();
		asteroid[numRoid].troidx = troidx;
		asteroid[numRoid].troidy = troidy;

		asteroid[numRoid].scaleX = .2;
		asteroid[numRoid].scaleY = .2;
		asteroid[numRoid].scaleZ = .2;

		asteroid[numRoid].red = 1.0f;
		asteroid[numRoid].green = 1.0f;
		asteroid[numRoid].blue = 1.0f;

		asteroid[numRoid].radius = 0.2f * sqrt(3.0f);

		asteroid[numRoid].isLarge = false;
		asteroid[numRoid].isMedium = false;
		asteroid[numRoid].destroyed = false;

		numRoid++;
	}
	else {
		destroyed = true;
	}
}

void Asteroid::score() {
	if (!isLarge && !isMedium) {
		playerScore += SMALL_ROID_SCORE;
		scoreSinceLastBonus += SMALL_ROID_SCORE;
	}
	else if (!isLarge && isMedium) {
		playerScore += MEDIUM_ROID_SCORE;
		scoreSinceLastBonus += MEDIUM_ROID_SCORE;
	}
	else {
		playerScore += LARGE_ASTEROID_SCORE;
		scoreSinceLastBonus += LARGE_ASTEROID_SCORE;
	}
}

//--------------------------------------------
// Player class
//--------------------------------------------

class Player {
	GLfloat oldax = .5f;
	GLfloat olday = 0.0f;
	GLfloat oldbx = -.3f;
	GLfloat oldby = .25f;
	GLfloat oldcx = -.3f;
	GLfloat oldcy = -.25f;
public:
	GLfloat		tplayerx;		// vector rotation For the player
	GLfloat		tplayery;		// vector rotation For the player
	GLfloat		vplayerx;		// velocity for moving the player in the x
	GLfloat		vplayery;		// velocity for moving the player in the y
	GLfloat		rplayer;		// angle of rotation of player.
	GLfloat		speed;			// speed
	GLfloat		rotationSpeed;	// Speed of rotation

	GLfloat		ax;				// Endpoint A
	GLfloat		bx;				// Endpoint B
	GLfloat		cx;				// Endpoint C
	GLfloat		ay;				// Endpoint A
	GLfloat		by;				// Endpoint B
	GLfloat		cy;				// Endponit C

	void display_player();
	void setup_player();
	void move_player();
	void rotate_player(int);
	void respawn();
	void accelerate();
	void take_shot();
}player;

void Player::display_player() {
	//Draw the Player Ship
	glPushMatrix();
	glTranslatef(tplayerx, tplayery, 0);
	glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);	// Top facing the screen
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);	// front facing to the right
	glRotatef(rplayer, 1.0f, 0.0f, 0.0f);    // Rotate The Triangle On The Y axis
	glScalef(.3, .6, .3);					  // Make ship twice as long
	glColor3f(1.0f, 1.0f, 0.0f);           // Set The Color To Blue
	glutSolidTetrahedron();
	glPopMatrix();
}

void Player::take_shot() {
	shot[bullets].position_shot(tplayerx, tplayery, vplayerx, vplayery, rplayer, false);
}

void Player::setup_player() {
	tplayerx = 0;
	tplayery = 0;
	vplayerx = 0;
	vplayery = 0;
	rplayer = 0;
	ax = .5f;
	ay = 0.0f;
	bx = -.3f;
	by = .25f;
	cx = -.3f;
	cy = -.25f;
}

void Player::move_player(){
	tplayerx += vplayerx;					// Move by velocity in x
	tplayery += vplayery;					// Move by velocity in y

	ax = tplayerx + sqrt(oldax * oldax + olday * olday) * cos(rplayer * (M_PI / 180));
	ay = tplayery + sqrt(oldax * oldax + olday * olday) * sin(rplayer * (M_PI / 180));
	bx = tplayerx + sqrt(oldbx * oldbx + oldby * oldby) * cos((rplayer + 135) * (M_PI / 180));
	by = tplayery + sqrt(oldbx * oldbx + oldby * oldby) * sin((rplayer + 135) * (M_PI / 180));
	cx = tplayerx + sqrt(oldcx * oldcx + oldcy * oldcy) * cos((rplayer + 225) * (M_PI / 180));
	cy = tplayery + sqrt(oldcx * oldcx + oldcy * oldcy) * sin((rplayer + 225) * (M_PI / 180));

	if (tplayerx > MAXRAND || tplayerx < MINRAND) {
		tplayerx = tplayerx * -1;
		ax = oldax + tplayerx;
		bx = oldbx + tplayerx;
		cx = oldcx + tplayerx;
	}
	if (tplayery > MAXRAND || tplayery < MINRAND) {
		tplayery = tplayery * -1;
		ay = olday + tplayery;
		by = oldby + tplayery;
		cy = oldcy + tplayery;
	}

	vplayerx -= vplayerx * 0.01f;
	vplayery -= vplayery * 0.01f;

}

void Player::rotate_player(int rotation){
	rplayer += rotation;

}

void Player::accelerate(){

	vplayerx += cos(rplayer * (M_PI / 180)) * SPEED;		// Move by velocity in x
	vplayery += sin(rplayer * (M_PI / 180)) * SPEED;		// Move by velocity in y
	if (sqrt(vplayerx * vplayerx + vplayery * vplayery) > MAX_SPEED){
		vplayerx = cos(rplayer * (M_PI / 180)) * MAX_SPEED;		// Move by velocity in x
		vplayery = sin(rplayer * (M_PI / 180)) * MAX_SPEED;		// Move by velocity in y
	}
}

//--------------------------------------------
// Alien Ship class
//--------------------------------------------

class Alien {
public:
	GLfloat		talienx;		// vector rotation For the alien
	GLfloat		talieny;		// vector rotation For the alien
	GLfloat		salien;			// x scale for the alien
	GLfloat		valienx;		// velocity for moving the alien in the x
	GLfloat		valieny;		// velocity for moving the alien in the y
	GLfloat		ralien;			// angle of rotation of alien.
	GLfloat		radius;			// Radius of alien center
	bool		isLarge;		// is alien ship large
	int lifetime = 0;			// age of alien ship
	bool alive = false;
	void display();
	void setup(bool);
	void move();
	void age();
	void shoot();
	void kill();
	bool isAlive();
	void score();
}alien;

void Alien::display() {
	glPushMatrix();
	glTranslatef(talienx, talieny, 0);
	glRotatef(90, 1.0f, 0.0f, 0.0f);    // Rotate The Triangle On The Y axis
	glRotatef(ralien, 0.0f, 0.0f, 1.0f);    // Rotate The Triangle On The Y axis
	glScalef(salien, salien, salien);					// Make it smaller
	glColor3f(1.0f, 0.0f, 0.0f);          // Set The Color To Red
	glutSolidTorus(.5, 1, 10, 10);
	glutSolidSphere(radius, 5, 5);
	glPopMatrix();
}

void Alien::setup(bool isLargeAlien) {
	alive = true;

	isLarge = isLargeAlien;

	if (isLarge) {
		salien = .6f;
		radius = 1;
	}
	else {
		salien = .3f;
		radius = .5;
	}

	int side = rand() % 2;

	if (side == 0) {
		side = -1;
	}

	talienx = MINRAND * side;
	talieny = (MAXRAND - MINRAND) * ((double)rand() / (double)RAND_MAX) + MINRAND;

	if (isLarge){
		valienx = ALIEN_SHIP_LARGE_SPEED * side;
	}
	else {
		valienx = ALIEN_SHIP_SMALL_SPEED * side;
	}
	valieny = (VMAXRAND - VMINRAND) * ((double)rand() / (double)RAND_MAX) + VMINRAND;
}

void Alien::move() {
	talienx += valienx;
	talieny += valieny;

	ralien += ALIEN_SHIP_ROTATION;

	if (talienx > MAXRAND || talienx < MINRAND) {
		alive = false;
	}
	if (talieny > MAXRAND || talieny < MINRAND) {
		talieny = talieny * -1;
	}

	if (lifetime > ALIEN_SHIP_MOVE_FREQ) {
		lifetime = 0;
		shoot();
		valieny = (VMAXRAND - VMINRAND) * ((double)rand() / (double)RAND_MAX) + VMINRAND;
	}
}

void Alien::age() {
	lifetime++;
}

void Alien::shoot() {

	int shotDir = (int)rand() % 101;
	double a;
	double s;
	double r;

	if (isLarge) {
		a = 30;
		s = 50;
		r = 100;
	}
	else {
		a = 40;
		s = 90;
		r = 100;
	}
	if (shotDir <= a) {
		GLfloat angle = atan2(asteroid[0].troidy - talieny, asteroid[0].troidx - talienx );
		angle = angle * (180 / M_PI);
		shot[bullets].position_shot(talienx, talieny, valienx, valieny, angle, true);
		bullets++;
	}
	else if (shotDir <= s) {
		GLfloat angle = atan2(player.tplayery - talieny, player.tplayerx - talienx);
		angle = angle * (180 / M_PI);
		shot[bullets].position_shot(talienx, talieny, valienx, valieny, angle, true);
		bullets++;
	}
	else {
		GLfloat angle = rand() / 360.0;
		shot[bullets].position_shot(talienx, talieny, valienx, valieny, angle, true);
		bullets++;
	}
}

void Alien::kill() {
	alive = false;
}

bool Alien::isAlive() {
	return alive;
}

void Alien::score() {
	if (isLarge) {
		playerScore += LARGE_ALIEN_SCORE;
		scoreSinceLastBonus += LARGE_ALIEN_SCORE;
	}
	else {
		playerScore += SMALL_ALIEN_SCORE;
		scoreSinceLastBonus += SMALL_ALIEN_SCORE;
	}
}

//--------------------------------------------
// Global Functions
//--------------------------------------------

void display_models()

{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();                   // Reset The View
	glTranslatef(0.0f, 0.0f, -9.0f);	// Move Left And Into The Screen

	glEnable(GL_CULL_FACE);						//Backface culling is enabled
	glCullFace(GL_BACK);						//Backface culling is enabled

	// Display player
	if (isAlive) {
		player.display_player();
	}

	// Display alien ship
	if (alien.isAlive()) {
		alien.display();
	}

	// Display asteroids
	if (numRoid > 0){
		for (int i = 0; i < numRoid; i++) {
			asteroid[i].display_roid();
		}
	}

	// Display bullets
	if (bullets > 0){
		for (int i = 0; i < bullets; i++) {
			shot[i].display_shot();
		}
	}

	// Display particles
	if (numPart > 0){
		for (int i = 0; i < numPart; i++) {
			particles[i].display();
		}
	}

	// Display score based on
	// http://stackoverflow.com/questions/2183270/what-is-the-easiest-way-to-print-text-to-screen-in-opengl
	glColor3f(1.0f, 1.0f, 1.0f);
	glRasterPos2f(4, 6);
	char string[50] = "Score: ";
	char score[50];
	itoa(playerScore, score, 10);
	strcat(string, score);
	int len, i;
	len = (int)strlen(string);
	for (i = 0; i < len; i++) {
		glutBitmapCharacter(GLUT_BITMAP_8_BY_13, string[i]);
	}

	glColor3f(1.0f, 1.0f, 1.0f);
	glRasterPos2f(-6, -5.8);
	char string2[50] = "Lives: ";
	char lives[50];
	itoa(playerLives, lives, 10);
	strcat(string2, lives);
	int len2, j;
	len2 = (int)strlen(string2);
	for (j = 0; j < len2; j++) {
		glutBitmapCharacter(GLUT_BITMAP_8_BY_13, string2[j]);
	}

	//Draw/Redraw
	glFlush();				// Flush OpenGL queue
	glutSwapBuffers();		// Display back buffer
}

int pnpoly(int nvert, float *vertx, float *verty, float testx, float testy)
{
	//http://stackoverflow.com/questions/11716268/point-in-polygon-algorithm
	int i, j, c = 0;
	for (i = 0, j = nvert - 1; i < nvert; j = i++) {
		if (((verty[i]>testy) != (verty[j]>testy)) &&
			(testx < (vertx[j] - vertx[i]) * (testy - verty[i]) / (verty[j] - verty[i]) + vertx[i]))
			c = !c;
	}
	return c;
}

void explode(GLfloat newX, GLfloat newY) {
	particles[numPart].setup(newX, newY, PART_SPEED, 0);
	numPart++;
	particles[numPart].setup(newX, newY, 0, PART_SPEED);
	numPart++;
	particles[numPart].setup(newX, newY, 0, PART_SPEED * -1);
	numPart++;
	particles[numPart].setup(newX, newY, PART_SPEED * -1, 0);
	numPart++;
	particles[numPart].setup(newX, newY, PART_SPEED * cos(45.0f), PART_SPEED * sin(45.0f));
	numPart++;
	particles[numPart].setup(newX, newY, PART_SPEED * cos(45.0f) * -1, PART_SPEED * sin(45.0f));
	numPart++;
	particles[numPart].setup(newX, newY, PART_SPEED * cos(45.0f), PART_SPEED * sin(45.0f) * -1);
	numPart++;
	particles[numPart].setup(newX, newY, PART_SPEED * cos(45.0f) * -1, PART_SPEED * sin(45.0f) * -1);
	numPart++;
}

void handle_menu(int ID)

//  This routine handles popup menu selections
//
//  ID:  Menu entry ID
{
	switch (ID) {
	case 0:				// Quit
		exit(0);
		break;
	case 1:
		isAlive = false;
		gameDelay = 33;	
		numRoid = 0;
		bullets = 0;
		numPart = 0;
		canShoot = true;
		canRotate = false;
		canAccelerate = false;
		rotateCW = false;
		levelComplete = true;
		roidToGenerate = 1;
		playerScore = 0;
		timeSinceLastAlien = 0;
		hasLrgAlienSpawned = false;
		playerLives = 4;
		scoreSinceLastBonus = 0;
		player.setup_player();
		alien.alive = false;
		break;
	}

}					// End routine handle_menu

bool pointInCircle(GLfloat sx, GLfloat sy, GLfloat x, GLfloat y, GLfloat r) {
	GLfloat deltaX = sx - x;
	GLfloat deltaY = sy - y;
	GLfloat d = (deltaX * deltaX) + (deltaY * deltaY);
	GLfloat r2 = r * r;
	if (d < r2) {
		return true;
	}
	return false;
}

void controlPlayer(unsigned char key, int mousex, int mousey){
	switch (key){
		case 'x':
			canAccelerate = true;
			break;
		case 'z':
			if (bullets < MAX_SHOTS && canShoot) {
				player.take_shot();
				bullets++;
				canShoot = false;
			}
			break;
		default:
			break;
	}
}

void keyRelease(unsigned char key, int mousex, int mousey) {
	switch (key){
	case 'x':
		canAccelerate = false;
		break;
	case 'z':
		canShoot = true;
		break;
	default:
		break;
	}
}

void specialRelease(int key, int mousex, int mousey) {
	switch (key){
		case GLUT_KEY_LEFT:
			canRotate = false;
		break;
		case GLUT_KEY_RIGHT:
			canRotate = false;
	default:
		break;
	}
}

void rotationControl(int key, int mousex, int mousey)
{
	switch (key){
	case GLUT_KEY_LEFT:
		//glutTimerFunc(playerDelay, playerTimerRot, PLAYER_ROT);
		canRotate = true;
		rotateCW = false;
		break;
	case GLUT_KEY_RIGHT:
		//glutTimerFunc(playerDelay, playerTimerRot, PLAYER_ROT * -1);
		canRotate = true;
		rotateCW = true;
		break;
	default:
		break;
	}
	glutPostRedisplay();
}

void gameTimer(int n)  //the "glutTimerFunc"
{

	// https://www.opengl.org/discussion_boards/showthread.php/181208-Rotate-oblect-using-glutTimerFunc

	// Spawn player if he is dead
	if (!isAlive && playerLives > 0) {
		player.setup_player();
		playerLives--;
		isAlive = true;
		
	}
	else if (playerLives == 0 && !isAlive) {
		canShoot = false;
	}
	// Update player's position
	else {
		player.move_player();
	}

	if (scoreSinceLastBonus >= BONUS_SHIP_SCORE) {
		playerLives++;
		scoreSinceLastBonus -= BONUS_SHIP_SCORE;
	}

	// Move alien if it is alive
	if (alien.isAlive()) {
		alien.move();
		alien.age();
	}
	else if (!alien.isAlive() && numRoid <= 4 && timeSinceLastAlien > ALIEN_TIMER && hasLrgAlienSpawned && playerScore >= MINSCORE) {
		alien.setup(false);
		timeSinceLastAlien = 0;
	}
	else if (!alien.isAlive() && numRoid <= 4 && timeSinceLastAlien > ALIEN_TIMER) {
		alien.setup(true);
		hasLrgAlienSpawned = true;
		timeSinceLastAlien = 0;
	}
	else{
		timeSinceLastAlien++;
	}

	// Collision Detection
	int temp = numRoid;

	// Does Alien collide with player
	if (pointInCircle(alien.talienx, alien.talieny, player.ax, player.ay, alien.radius) || pointInCircle(alien.talienx, alien.talieny, player.bx, player.by, alien.radius) || pointInCircle(alien.talienx, alien.talieny, player.cx, player.cy, alien.radius)) {
		if (alien.alive) {
			alien.kill();
			isAlive = false;
			explode(alien.talienx, alien.talieny);
			explode(player.tplayerx, player.tplayery);
		}
	}

	// Does Alien collide with asteroid
	if (alien.isAlive()) {
		for (int i = 0; i <= numRoid; i++) {

			double distanceBetweenCenters = sqrt(pow(asteroid[i].troidx - alien.talienx, 2) + pow(asteroid[i].troidy - alien.talieny, 2));
			double diffOfRad = abs(asteroid[i].radius - alien.radius);
			double sumOfRad = asteroid[i].radius + alien.radius;

			if (diffOfRad <= distanceBetweenCenters && distanceBetweenCenters <= sumOfRad) {
				alien.kill();
				asteroid[i].split();
				explode(alien.talienx, alien.talieny);
				explode(asteroid[i].troidx, asteroid[i].troidy);
			}
		}

	}

	// Shot collision
	for (int i = 0; i < bullets; i++) {
		// Did asteroid collide with shot
		for (int j = 0; j < temp; j++) {
			if (pointInCircle(asteroid[j].troidx, asteroid[j].troidy, shot[i].tShotx, shot[i].tShoty, asteroid[j].radius)) {
				if (!shot[i].alienShot) {
					asteroid[j].score();
				}
				asteroid[j].split();
				explode(asteroid[j].troidx, asteroid[j].troidy);
				shot[i].destroyed = true;
			}
		}

		// Did player collide with alien shot
		GLfloat playerXVert[3] = { player.ax, player.bx, player.cx };
		GLfloat playerYVert[3] = { player.ay, player.by, player.cy };

		int edges = pnpoly(3, playerXVert, playerYVert, shot[i].tShotx, shot[i].tShoty);
		if (isAlive && shot[i].alienShot && edges >= 1) {
			isAlive = false;
			explode(player.tplayerx, player.tplayery);
		}

		// Did alien ship collide with shot
		if (pointInCircle(alien.talienx, alien.talieny, shot[i].tShotx, shot[i].tShoty, alien.radius)) {
			if (!shot[i].alienShot && alien.isAlive()) {
				alien.score();
				alien.kill();
				explode(alien.talienx, alien.talieny);
				shot[i].destroyed = true;
			}
		}
	}

	// Player collision with asteroid
	for (int i = 0; i < temp; i++) {
		GLfloat x1 = player.ax;
		GLfloat y1 = player.ay;
		GLfloat x2 = player.bx;
		GLfloat y2 = player.by;
		GLfloat sy;
		GLfloat deltaX = x2 - x1;
		GLfloat deltaY = y2 - y1;
		GLfloat d = deltaX * deltaX + deltaY * deltaY;
		GLfloat D = (x1 - asteroid[i].troidx)*(y2 - asteroid[i].troidy) - (x2 - asteroid[i].troidx)*(y1 - asteroid[i].troidy);
		GLfloat delta = (asteroid[i].radius * asteroid[i].radius) * d - (D * D);
		if (deltaY < 0) {
			sy = -1;
		}
		else {
			sy = 1;
		}
		if (delta >= 0) {

			GLfloat X1 = ((D * deltaY + sy * deltaX * sqrt(asteroid[i].radius * asteroid[i].radius * d - D * D)) / d) + player.tplayerx;
			GLfloat Y1 = ((-1 * D * deltaX + abs(deltaY) * sqrt(asteroid[i].radius * asteroid[i].radius * d - D * D)) / d) + player.tplayery;
			GLfloat X2 = ((D * deltaY - sy * deltaX * sqrt(asteroid[i].radius * asteroid[i].radius * d - D * D)) / d) + player.tplayerx;
			GLfloat Y2 = ((-1 * D * deltaX - abs(deltaY) * sqrt(asteroid[i].radius * asteroid[i].radius * d - D * D)) / d) + player.tplayery;

			if (pointInCircle(asteroid[i].troidx, asteroid[i].troidy, X1, Y1, asteroid[i].radius) || pointInCircle(asteroid[i].troidx, asteroid[i].troidy, X2, Y2, asteroid[i].radius)){
				explode(asteroid[i].troidx, asteroid[i].troidy);
				asteroid[i].split();
				explode(player.tplayerx, player.tplayery);
				isAlive = false;
			}
		}

		x1 = player.cx;
		y1 = player.cy;
		x2 = player.bx;
		y2 = player.by;
		sy;
		deltaX = x2 - x1;
		deltaY = y2 - y1;
		d = deltaX * deltaX + deltaY * deltaY;
		D = (x1 - asteroid[i].troidx)*(y2 - asteroid[i].troidy) - (x2 - asteroid[i].troidx)*(y1 - asteroid[i].troidy);
		delta = (asteroid[i].radius * asteroid[i].radius) * d - (D * D);
		if (deltaY < 0) {
			sy = -1;
		}
		else {
			sy = 1;
		}
		if (delta >= 0) {

			GLfloat X1 = ((D * deltaY + sy * deltaX * sqrt(asteroid[i].radius * asteroid[i].radius * d - D * D)) / d) + player.tplayerx;
			GLfloat Y1 = ((-1 * D * deltaX + abs(deltaY) * sqrt(asteroid[i].radius * asteroid[i].radius * d - D * D)) / d) + player.tplayery;
			GLfloat X2 = ((D * deltaY - sy * deltaX * sqrt(asteroid[i].radius * asteroid[i].radius * d - D * D)) / d) + player.tplayerx;
			GLfloat Y2 = ((-1 * D * deltaX - abs(deltaY) * sqrt(asteroid[i].radius * asteroid[i].radius * d - D * D)) / d) + player.tplayery;

			if (pointInCircle(asteroid[i].troidx, asteroid[i].troidy, X1, Y1, asteroid[i].radius) || pointInCircle(asteroid[i].troidx, asteroid[i].troidy, X2, Y2, asteroid[i].radius)){
				explode(asteroid[i].troidx, asteroid[i].troidy);
				asteroid[i].split();
				explode(player.tplayerx, player.tplayery);
				isAlive = false;
			}
		}

		x1 = player.cx;
		y1 = player.cy;
		x2 = player.ax;
		y2 = player.ay;
		sy;
		deltaX = x2 - x1;
		deltaY = y2 - y1;
		d = deltaX * deltaX + deltaY * deltaY;
		D = (x1 - asteroid[i].troidx)*(y2 - asteroid[i].troidy) - (x2 - asteroid[i].troidx)*(y1 - asteroid[i].troidy);
		delta = (asteroid[i].radius * asteroid[i].radius) * d - (D * D);
		if (deltaY < 0) {
			sy = -1;
		}
		else {
			sy = 1;
		}
		if (delta >= 0) {

			GLfloat X1 = ((D * deltaY + sy * deltaX * sqrt(asteroid[i].radius * asteroid[i].radius * d - D * D)) / d) + player.tplayerx;
			GLfloat Y1 = ((-1 * D * deltaX + abs(deltaY) * sqrt(asteroid[i].radius * asteroid[i].radius * d - D * D)) / d) + player.tplayery;
			GLfloat X2 = ((D * deltaY - sy * deltaX * sqrt(asteroid[i].radius * asteroid[i].radius * d - D * D)) / d) + player.tplayerx;
			GLfloat Y2 = ((-1 * D * deltaX - abs(deltaY) * sqrt(asteroid[i].radius * asteroid[i].radius * d - D * D)) / d) + player.tplayery;

			if (pointInCircle(asteroid[i].troidx, asteroid[i].troidy, X1, Y1, asteroid[i].radius) || pointInCircle(asteroid[i].troidx, asteroid[i].troidy, X2, Y2, asteroid[i].radius)){
				explode(asteroid[i].troidx, asteroid[i].troidy);
				asteroid[i].split();
				explode(player.tplayerx, player.tplayery);
				isAlive = false;
			}
		}

	}

	// Clean any shots up that have collided
	for (int i = 0; i < bullets; i++) {
		if (shot[i].destroyed) {
			for (int j = i; j < bullets - 1; j++) {
				shot[j] = shot[j + 1];
			}
			bullets--;
		}
	}

	// Clean up any asteroids that have been destroyed
	// Any asteroids hit and are large can split
	for (int i = 0; i < temp; i++) {
		if (asteroid[i].destroyed) {
			for (int j = i; j < numRoid - 1; j++) {
				asteroid[j] = asteroid[j + 1];
			}
			numRoid--;
		}
	}

	// If level is complete, generate a new level
	if (levelComplete) {
		hasLrgAlienSpawned = false;
		for (int i = 0; i < roidToGenerate; i++) {
			asteroid[numRoid].setup_roid();
			numRoid++;
		}
		levelComplete = false;
	}
	// Once all asteroids are destoryed, a new level must be generated.
	if (numRoid == 0) {
		levelComplete = true;
		roidToGenerate++;
		if (roidToGenerate > MAXLARGEROID) {
			roidToGenerate = MAXLARGEROID;
		}
	}
	// If there are still asteroids, update their positions
	else if (numRoid > 0){
		for (int i = 0; i < numRoid; i++) {
			asteroid[i].move_roid();
		}
	}

	// If the player is input a command to rotate, update the rotation.
	if (canRotate){
		if (!rotateCW) {
			player.rotate_player(PLAYER_ROT);
		}
		else if (rotateCW){
			player.rotate_player(PLAYER_ROT * -1);
		}
	}

	// If the player is input a command to accelerate, update the velocities.
	if (canAccelerate) {
		player.accelerate();
	}

	// move particles
	if (numPart > 0){
		for (int i = 0; i < numPart; i++) {
			particles[i].move();
			particles[i].age();

			if (particles[i].lifetime >= PART_LIFETIME) {
				for (int j = i; j < numPart - 1; j++) {
					particles[j] = particles[j + 1];
				}
				numPart--;
				i--;
			}
		}
	}

	// Facilitates shot count of 4. Player has to press space for every shot
	if (bullets > 0){
		for (int i = 0; i < bullets; i++) {
			shot[i].move_shot();
		}
	}

	// Bullets expire when they reach their max lifetime
	for (int k = 0; k < bullets; k++) {
		if (shot[k].age() >= SHOT_LIFETIME) {
			for (int j = k; j < bullets - 1; j++) {
				shot[j] = shot[j + 1];
			}
			bullets--;
		}
	}

	// The display must be redrawn
	glutPostRedisplay();

	// Loop back to the beginning
	glutTimerFunc(gameDelay, gameTimer, n);
}

void main(int argc, char *argv[])
{
	srand(time(NULL)); //Seed with time

	float  amb[] = { 0, 0, 0, 1 };	// Ambient material property
	float  lt_amb[] = { .2, .2, .2, 1 };	// Ambient light property
	float  lt_dif[] = { .8, .8, .8, 1 };	// Ambient light property
	float  lt_pos[] = {			// Light position
		0, .39392, .91914, 0
	};
	float  lt_spc[] = { 0, 0, 0, 1 };	// Specular light property

	glutInit(&argc, argv);		// Initialize GLUT
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(800, 800);
	glutCreateWindow("Asteroids");

	glutDisplayFunc(display_models);	// Setup GLUT callbacks
	glutTimerFunc(gameDelay, gameTimer, 0);
	glutKeyboardFunc(controlPlayer);
	glutKeyboardUpFunc(keyRelease);
	glutSpecialFunc(rotationControl);
	glutSpecialUpFunc(specialRelease);

	glutCreateMenu(handle_menu);	// Setup GLUT popup menu
	glutAddMenuEntry("Quit", 0);
	glutAddMenuEntry("Start New Game", 1);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	glMatrixMode(GL_PROJECTION);	// Setup perspective projection
	glLoadIdentity();
	gluPerspective(70, 1, 1, 40);

	glMatrixMode(GL_MODELVIEW);		// Setup model transformations
	glLoadIdentity();
	gluLookAt(0, 0, 5, 0, 0, -1, 0, 1, 0);

	// http://www.csc.ncsu.edu/faculty/healey/csc461/assn_2/lighting.cpp
	//  Set default ambient light in scene

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);

	//  Set Light 0 position, ambient, diffuse, specular intensities

	glLightfv(GL_LIGHT0, GL_POSITION, lt_pos);
	glLightfv(GL_LIGHT0, GL_AMBIENT, lt_amb);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lt_dif);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lt_spc);

	//  Enable Light 0 and GL lighting

	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);

	glShadeModel(GL_FLAT);		// Flat shading
	glEnable(GL_NORMALIZE);		// Normalize normals

	glClearDepth(1.0);			// Setup background colour
	glClearColor(0, 0, 0, 0);

	// https://www.opengl.org/discussion_boards/showthread.php/171207-glMaterial
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE); // Which properties ojects have
	glEnable(GL_COLOR_MATERIAL); // Give objects color

	glClearDepth( 1.0 );
	glEnable(GL_DEPTH_TEST);

	glutSetOption(			// Return to main loop on window close
		GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

	glutMainLoop();			// Enter GLUT main loop
}