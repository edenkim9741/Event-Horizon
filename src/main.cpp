#include <GL/glut.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <random>
#include <glm/gtc/random.hpp>
#include <glm/glm.hpp>
#include <omp.h>
#include <glm/gtc/type_ptr.hpp>

const int numRays = 1000;
static int Time = 0;

//Body* selectedBody = nullptr;
int selectedBodyIndex = -1;

struct Body {
	glm::vec3 position;
	float mass;
	float radius;
	glm::vec3 color;
	std::vector<Body*> satellites = {};
	glm::vec3 relativePosition = { 0.0f, 0.0f, 0.0f };
	float orbitRadius = 0.0f;
	float orbitFrequency = 0.0f;
	float rotationFrequency = 0.0f;
};

// --- 전역 변수 설정 ---
std::vector<std::vector<glm::vec3>> rayPaths(numRays);
std::vector<Body*> bodies;
float lightSpeed = 25.0f;
float dt = 0.01f;

std::vector<glm::vec3> velocities(numRays);

glm::vec4 lightPosition = { -10.0f, 10.0f, -7.0f, 1.0f };

glm::mat4 currentModelMatrix;


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
	Body* planet1 = new Body{ {0.0f, 0.0f, 0.0f}, 300.0f, 1.5f, {0.4f, 0.4f, 0.8f}, {}, {6.0f, 0.0f,0.0f}, 12.0f, 0.2f };
	Body* neutronStar = new Body{ {0.0f, 0.0f, 0.0f}, 500.0f, 3.0f, {0.1f, 0.1f, 0.1f},{planet1}, {20.0f, 0.0f, 0.0f}, 7.0f, 3.0f };
	Body* blackhole = new Body{ {0.0f, 0.0f, 0.0f}, 800.0f, 5.0f, {0.0f, 0.0f, 0.0f}, {neutronStar}, {0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.005f };


	bodies.push_back(blackhole);
	bodies.push_back(neutronStar);
	bodies.push_back(planet1);
}

void makeVelocities() {
	//#pragma omp parallel for
	for (int i = 0; i < numRays; i++)
	{
		velocities[i] = glm::sphericalRand(1.0f) * lightSpeed;
	}
}


void simulateRay(glm::vec4 light_pos) {
	rayPaths.resize(numRays);
	std::vector<glm::vec3> temp_velocities(velocities);

#pragma omp parallel for
	for (int i = 0; i < numRays; i++)
	{
		glm::vec3 position = light_pos;

		std::vector<glm::vec3> rayPath;
		rayPath.reserve(10000);

		// 안전을 위해 최대 반복 횟수 설정 (무한 루프 방지)
		int maxSteps = 100000;
		int step = 0;

		while (step < maxSteps) {

			glm::vec3 totalAcceleration = { 0, 0, 0 };
			bool absorbed = false;

			// 가장 가까운 천체와의 거리 찾기 (가변 스텝을 위해)
			float minBodyDistSq = 1e9f;

			// 모든 천체에 대해 중력 계산
			for (const auto& body : bodies) {
				glm::vec3 dir = body->position - position;
				float distSq = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
				float dist = sqrt(distSq);

				if (dist < body->radius) {
					absorbed = true;
					break;
				}

				if (distSq < minBodyDistSq) minBodyDistSq = distSq;

				float accelMag = body->mass / std::max(distSq, 0.1f);
				totalAcceleration = totalAcceleration + (dir * (1.0f / dist) * accelMag);
			}

			if (absorbed) break;

			float currentDt = dt;
			if (minBodyDistSq > 1000.0f) currentDt *= 5.0f;  // 거리가 꽤 멀면 5배 빠르게
			if (minBodyDistSq > 10000.0f) currentDt *= 10.0f; // 아주 멀면 더 빠르게

			// 오일러 적분
			temp_velocities[i] = temp_velocities[i] + totalAcceleration * currentDt;
			position = position + temp_velocities[i] * currentDt;
			// --- [핵심 수정] 종료 범위 확장 ---
			// 기존 500.0f -> 4000.0f 로 대폭 확장
			if (position.x > 4000.0f || position.x < -4000.0f ||
				position.y > 4000.0f || position.y < -4000.0f ||
				position.z > 4000.0f || position.z < -4000.0f) break;

			rayPath.push_back(position);

			step++;
		}

		//rayPath.insert(rayPath.end(), tempRayPath.rbegin(), tempRayPath.rend());
		rayPaths[i] = std::move(rayPath);
	}
}
// 조명 초기화
void initLighting() {
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL); // 색상이 조명에 반응하도록 설정

	GLfloat light_pos[] = { lightPosition.x, lightPosition.y, lightPosition.z };
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
	makeVelocities();
}

void drawRay(std::vector<glm::vec3> rayPath) {
	// 1. 현재 OpenGL 상태 저장 (선택 사항이나 권장됨)
	glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_LIGHTING); // 광선은 자체 발광
	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glDepthMask(GL_FALSE);

	glLineWidth(1.5f);
	glBegin(GL_LINE_STRIP);

	// 알파값 0.1은 매우 투명하므로 잘 안 보일 수 있습니다. 
	// 테스트를 위해 0.3 ~ 0.5 정도로 조정해보는 것도 좋습니다.
	glColor4f(1.0f, 0.9f, 0.2f, 0.2f);

	for (const auto& p : rayPath) {
		glVertex3f(p.x, p.y, p.z);
	}

	glEnd();

	// 3. 상태 복구 (또는 glDisable로 끄기)
	glPopAttrib();
	// glPopAttrib을 안 쓴다면 아래처럼 직접 꺼주세요
	// glDisable(GL_BLEND);
	// glDepthMask(GL_TRUE);
	// glEnable(GL_LIGHTING);
}

void pickBody(int mouseX, int mouseY) {
	GLdouble modelview[16];
	GLdouble projection[16];
	GLint viewport[4];

	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);
	glGetIntegerv(GL_VIEWPORT, viewport);

	selectedBodyIndex = -1; // 선택 초기화
	double minDepth = 1.0;  // 가장 앞에 있는 물체를 선택하기 위함

	for (int i = 0; i < bodies.size(); ++i) {
		double winX, winY, winZ;

		// 천체의 3D 위치를 2D 화면 좌표(winX, winY)로 투영
		gluProject(bodies[i]->position.x, bodies[i]->position.y, bodies[i]->position.z,
			modelview, projection, viewport,
			&winX, &winY, &winZ);

		double projRadius;

		// 천체의 반지름을 화면에 투영
		gluProject(bodies[i]->position.x + bodies[i]->radius, bodies[i]->position.y, bodies[i]->position.z,
			modelview, projection, viewport,
			&projRadius, &winY, &winZ);

		projRadius = fabs(projRadius - winX); // 반지름 크기 계산

		double dist = sqrt(pow(mouseX - winX, 2) + pow((viewport[3] - mouseY) - winY, 2));

		// (winZ가 작을수록 카메라에 가까움)
		if (dist < projRadius && winZ < minDepth) {
			selectedBodyIndex = i;
			minDepth = winZ;
		}
	}

	if (selectedBodyIndex != -1) {
		std::cout << "Selected Body Index: " << selectedBodyIndex
			<< ", Mass: " << bodies[selectedBodyIndex]->mass << std::endl;
	}
	else {
		std::cout << "Selection Cleared" << std::endl;
	}
}

void updatePositions(Body* body) {
	glPushMatrix();
	glRotatef((GLfloat)(Time * body->rotationFrequency), 0.0f, 1.0f, 0.0f);
	glTranslatef(body->relativePosition.x, body->relativePosition.y, body->relativePosition.z);

	glm::mat4 modelMatrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, glm::value_ptr(modelMatrix));
	body->position = glm::vec3(modelMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

	glRotatef((GLfloat)(Time * body->orbitFrequency * 36.0f), 0.0f, 1.0f, 0.0f);

	for (const auto& satellite : body->satellites) {
		updatePositions(satellite);
	}

	glPopMatrix();
}

void drawBodies(Body* body) {
	glPushMatrix();
	glColor3f(body->color.x, body->color.y, body->color.z);
	glRotatef((GLfloat)(Time * body->rotationFrequency), 0.0f, 1.0f, 0.0f);
	glTranslatef(body->relativePosition.x, body->relativePosition.y, body->relativePosition.z);
	glRotatef((GLfloat)(Time * body->orbitFrequency), 0.0f, 1.0f, 0.0f);

	glutSolidSphere(body->radius, 30, 30);
	
	glDisable(GL_LIGHTING);
	glColor4f(body->color.x, body->color.y, body->color.z, 0.3f);
	glutWireSphere(body->radius * 1.1f, 15, 15);

	if (selectedBodyIndex >= 0 && (void*)body == (void*)bodies[selectedBodyIndex]) {
		glColor3f(0.0f, 1.0f, 0.0f); // 녹색
		glLineWidth(2.0f);
		glutWireSphere(body->radius * 1.3f, 10, 10); // 천체보다 조금 큰 박스
		glLineWidth(1.0f);
	}
	glEnable(GL_LIGHTING);

	for (const auto& satellite : body->satellites) {
		drawBodies(satellite);
	}

	glPopMatrix();
}

void display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	updatePositions(bodies[0]);
	simulateRay(lightPosition);

	glLoadIdentity();

	float camX = cameraDistance * sin(cameraAngleY) * cos(cameraAngleX);
	float camY = cameraDistance * sin(cameraAngleX);
	float camZ = cameraDistance * cos(cameraAngleY) * cos(cameraAngleX);

	gluLookAt(camX, camY, camZ, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

	drawBodies(bodies[0]);

	for (const auto& rayPath : rayPaths) {
		drawRay(rayPath);
	}


	glutSwapBuffers();
}
// [수정] 마우스 함수
void mouseFunc(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			// 클릭 시 선택 시도
			pickBody(x, y);

			// 만약 천체를 선택하지 않았다면 드래그(회전) 모드로 진입
			if (selectedBodyIndex == -1) {
				isDragging = true;
				lastMouseX = x;
				lastMouseY = y;
			}
		}
		else {
			isDragging = false;
		}
	}
	// 줌 인/아웃
	if (button == 3) cameraDistance -= 2.0f;
	if (button == 4) cameraDistance += 2.0f;
	if (cameraDistance < 10.0f) cameraDistance = 10.0f;

	glutPostRedisplay();
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

// 키보드 제어 함수
// [추가] 키보드 특수 키(화살표) 콜백 함수
void specialKeyFunc(int key, int x, int y) {
	if (selectedBodyIndex != -1) {
		bool changed = false;
		if (key == GLUT_KEY_UP) {
			bodies[selectedBodyIndex]->mass += 20.0f;
			changed = true;
		}
		else if (key == GLUT_KEY_DOWN) {
			bodies[selectedBodyIndex]->mass -= 5.0f;
			// 질량이 음수가 되지 않도록 방지 (원한다면 제거 가능)
			if (bodies[selectedBodyIndex]->mass < 0.0f) bodies[selectedBodyIndex]->mass = 0.0f;
			changed = true;
		}

		if (changed) {
			std::cout << "Body " << selectedBodyIndex << " Mass changed to: "
				<< bodies[selectedBodyIndex]->mass << std::endl;

			std::cout << "Recalculating simulation..." << std::endl;
			glutPostRedisplay(); // 화면 갱신
		}
	}
}


void MyTimer(int Value)
{
	Time = (Time + Value) % INT_MAX;

	glutPostRedisplay();
	glutTimerFunc(30, MyTimer, Value);
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
	glutSpecialFunc(specialKeyFunc);
	glutTimerFunc(30, MyTimer, 1);

	glutMainLoop();
	return 0;
}