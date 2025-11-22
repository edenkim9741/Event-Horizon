#include <GL/glut.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <random>

const int numRays = 1;
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<int> dis(-40, 40);


// --- 구조체 정의 ---
struct Vec3 {
	float x, y, z;
	Vec3 operator+(const Vec3& v) const { return { x + v.x, y + v.y, z + v.z }; }
	Vec3 operator-(const Vec3& v) const { return { x - v.x, y - v.y, z - v.z }; }
	Vec3 operator*(float s) const { return { x * s, y * s, z * s }; }
};

struct Body {
	Vec3 position;
	float mass;
	float radius;
	Vec3 color;
};

// --- 전역 변수 설정 ---
std::vector<std::vector<Vec3>> rayPaths;
std::vector<Body> bodies;
float lightSpeed = 25.0f;
float dt = 0.005f;

// 카메라 제어 변수0, 99
float cameraAngleX = 0.0f;
float cameraAngleY = 0.0f;
float cameraDistance = 60.0f;
int lastMouseX, lastMouseY;
bool isDragging = false;

// --- 함수 정의 ---

// 초기 장면 설정 (천체 배치)
void setupScene() {
	// 중앙의 거대 블랙홀
	bodies.push_back({ {0.0f, 0.0f, 0.0f}, 800.0f, 3.0f, {0.1f, 0.1f, 0.1f} });
	// 주변의 작은 중성자별들
	bodies.push_back({ {15.0f, 5.0f, 5.0f}, 300.0f, 1.5f, {0.4f, 0.4f, 0.8f} });
	bodies.push_back({ {-10.0f, -8.0f, 10.0f}, 250.0f, 1.2f, {0.8f, 0.4f, 0.4f} });
}

// 광선 시뮬레이션 (핵심 물리 엔진 - 다중 중력)
void simulateRay() {
	rayPaths.clear();

	for (int i = 0; i < numRays; i++)
	{
		//Vec3 position = { dis(gen), dis(gen), dis(gen) }; // 초기 위치
		//Vec3 random_velocity = { lightSpeed, dis(gen), dis(gen) };
		//Vec3 velocity[2] = {random_velocity, random_velocity*(-1.0)}; // 초기 방향 (+, -)


		Vec3 position = { -10.0f, 10.0f, -5.0f }; // 시작점
		Vec3 velocity[2] = {{lightSpeed, 0.0f, 0.0f}, {-lightSpeed, 0.0f, 0.0f}}; // 초기 방향 (+, -)

		rayPaths.push_back(std::vector<Vec3>{});

		rayPaths[i].push_back(position);

		for (int j = 0; j < 1; j++) {
			position = { -10.0f, 10.0f, -5.0f };
			while(true) {
				Vec3 totalAcceleration = { 0, 0, 0 };
				bool absorbed = false;

				// 모든 천체에 대해 중력 계산
				for (const auto& body : bodies) {
					Vec3 dir = body.position - position; // 광선에서 천체로 향하는 벡터
					float distSq = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
					float dist = sqrt(distSq);

					if (dist < body.radius) { // 충돌 체크
						absorbed = true;
						break;
					}

					// F = G * M / r^2 (방향은 정규화된 dir 벡터)
					// 가속도 크기 = mass / distSq (G는 1로 가정)
					float accelMag = body.mass / std::max(distSq, 0.1f); // 0 나누기 방지
					totalAcceleration = totalAcceleration + (dir * (1.0f / dist) * accelMag);
				}

				if (absorbed) break;
				// 오일러 적분
				velocity[j] = velocity[j] + totalAcceleration * dt;
				position = position + velocity[j] * dt;

				rayPaths[i].push_back(position);

				if (position.x > 500.0f || position.x < -500.0f ||
					position.y > 500.0f || position.y < -500.0f ||
					position.z > 500.0f || position.z < -500.0f) break; // 너무 멀어지면 종료
			}
		}
	}
}

// 조명 초기화
void initLighting() {
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL); // 색상이 조명에 반응하도록 설정

	GLfloat light_pos[] = { 50.0f, 50.0f, 50.0f, 1.0f };
	GLfloat white_light[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat ambient_light[] = { 0.2f, 0.2f, 0.2f, 1.0f };

	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
	glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient_light);

	// 재질 반사광 설정
	GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat mat_shininess[] = { 50.0f };
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
}

void init() {
	glClearColor(0.05f, 0.05f, 0.1f, 1.0f); // 약간 푸른 우주 배경
	glEnable(GL_DEPTH_TEST);
	initLighting();
	setupScene();
	simulateRay();
}

void drawRay(std::vector<Vec3> rayPath) {
	glDisable(GL_LIGHTING); // 광선은 자체 발광처럼 보이게 조명 끔
	glLineWidth(1.5f);
	glBegin(GL_LINE_STRIP);
	glColor4f(1.0f, 0.9f, 0.2f, 0.1f); // 밝은 황금색 광선
	for (const auto& p : rayPath) {
		glVertex3f(p.x, p.y, p.z);
	}

	glEnd();
	glEnable(GL_LIGHTING);

}

void display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	// 카메라 위치 계산 (Spherical Coordinates)
	float camX = cameraDistance * sin(cameraAngleY) * cos(cameraAngleX);
	float camY = cameraDistance * sin(cameraAngleX);
	float camZ = cameraDistance * cos(cameraAngleY) * cos(cameraAngleX);
	gluLookAt(camX, camY, camZ, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);


	// 1. 모든 천체 그리기
	for (const auto& body : bodies) {
		glColor3f(body.color.x, body.color.y, body.color.z);
		glPushMatrix();
		glTranslatef(body.position.x, body.position.y, body.position.z);
		glutSolidSphere(body.radius, 30, 30);
		// 와이어프레임으로 약간의 빛무리 효과
		glDisable(GL_LIGHTING);
		glColor4f(body.color.x, body.color.y, body.color.z, 0.3f);
		glutWireSphere(body.radius * 1.1f, 15, 15);
		glEnable(GL_LIGHTING);
		glPopMatrix();
	}

	// 2. 광선 궤적 그리기
	for (int i = 0; i < numRays; i++) {
		drawRay(rayPaths[i]);
	}


	glutSwapBuffers();
}

// --- 마우스 제어 콜백 함수들 ---
void mouseFunc(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			isDragging = true;
			lastMouseX = x;
			lastMouseY = y;
		}
		else {
			isDragging = false;
		}
	}
	// 마우스 휠로 줌인/줌아웃 (GLUT 버전에 따라 다를 수 있음)
	if (button == 3) cameraDistance -= 2.0f;
	if (button == 4) cameraDistance += 2.0f;
	if (cameraDistance < 10.0f) cameraDistance = 10.0f;
}

void motionFunc(int x, int y) {
	if (isDragging) {
		cameraAngleY += (x - lastMouseX) * 0.01f;
		cameraAngleX += (y - lastMouseY) * 0.01f;

		// 수직 각도 제한
		if (cameraAngleX > 1.5f) cameraAngleX = 1.5f;
		if (cameraAngleX < -1.5f) cameraAngleX = -1.5f;

		lastMouseX = x;
		lastMouseY = y;
		glutPostRedisplay();
	}
}

void reshape(int w, int h) {
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0f, (float)w / h, 1.0f, 200.0f); // 시야각 넓힘
	glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(1024, 768);
	glutCreateWindow("3D Multi-Body Gravitational Lensing Ray");

	init();

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouseFunc);
	glutMotionFunc(motionFunc);

	glutMainLoop();
	return 0;
}