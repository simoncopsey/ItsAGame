#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glew32s.lib")
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "glfw3dll.lib")

#include <iostream>
#include <process.h>
#include <map>
#include <list>
#include <memory>
#include <string>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// TODO: new class that inherits from actor and listener, so actor does not inherit from listener directly
// TODO: onUpdate() should include millisecond offset
// TODO: both actor and listener have onUpdate() methods
// TODO: destructor / memory cleanup

// A useful macro
#define SAFE_DELETE(p) { if (p) { delete (p); (p) = NULL; } };

enum EVENT_TYPE {
	KEY_PRESS = 0,
};

class Listener {
public:
	virtual void onUpdate(const EVENT_TYPE _event_type, const std::string _message) = 0;
};

struct Position {
	float x_position;
	float y_position;
	float rotation_angle;

	Position(float _x_position, float _y_position, float _rotation_angle) :
		x_position(_x_position), y_position(_y_position), rotation_angle(_rotation_angle) {}
};

struct Color {
	GLubyte red;
	GLubyte green;
	GLubyte blue;

	Color(GLubyte _red, GLubyte _green, GLubyte _blue) : red(_red), green(_green), blue(_blue) {};
};

class EventManager {
private:
	std::map<EVENT_TYPE, std::list<std::weak_ptr<Listener>>> m_listeners;
	EventManager() {};
public:
	static EventManager& getInstance() {
		static EventManager instance;
		return instance;
	}
	EventManager(EventManager const&) = delete;
	void operator=(EventManager const&) = delete;

	void registerListener(const EVENT_TYPE _event_type, const std::weak_ptr<Listener> _listener) {
		if (m_listeners.count(_event_type) > 0) {
			EventManager::m_listeners.find(_event_type)->second.push_back(_listener);
		} else {
			std::list<std::weak_ptr<Listener>> listeners;
			listeners.push_back(_listener);
			EventManager::m_listeners.emplace(_event_type, listeners);
		}
	}

	void publishEvent(const EVENT_TYPE _event_type, const std::string _message) {
		auto result = EventManager::m_listeners.find(_event_type);
		if (result != EventManager::m_listeners.end()) {
			std::list<std::weak_ptr<Listener>> listeners = result->second;
			for (const std::weak_ptr<Listener> listener_wp : listeners) {
				if (!listener_wp.expired()) {
					std::shared_ptr<Listener> listener_sp = std::shared_ptr<Listener>(listener_wp);
					Listener *listener = listener_sp.get();
					listener->onUpdate(_event_type, _message);
				}
			}
		}
	}
};

class GameApplicationLayer {
private:
	static const GLint triangle_vertices[];
	static const GLubyte triangle_indices[];

public:
	static void drawTriangle(const Position _position, const Color _color) {
		GLubyte triangle_colors[] = {
			_color.red, _color.green, _color.blue,
			_color.red, _color.green, _color.blue,
			_color.red, _color.green, _color.blue
		};

		glVertexPointer(3, GL_INT, 0, triangle_vertices);
		glColorPointer(3, GL_UNSIGNED_BYTE, 0, triangle_colors);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
			glLoadIdentity();
			glRotatef(_position.rotation_angle, 0.0f, 0.0f, 1.0f);
			glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_BYTE, GameApplicationLayer::triangle_indices);
		glPopMatrix();
	}
};

const GLint GameApplicationLayer::triangle_vertices[] = {
	0, 1, 0,
	1, -1, 0,
	-1, -1, 0
};

const GLubyte GameApplicationLayer::triangle_indices[] = {
	0, 1, 2
};

class Actor : public Listener {
private:
	int m_id;
	Position m_position;
	Color m_color;
	std::map<int, int> m_components;

public:
	void onUpdate(const EVENT_TYPE _event_type, const std::string _message) {
		if (EVENT_TYPE::KEY_PRESS == _event_type && "UP" == _message) {
			m_position.rotation_angle += 10;
		} else if (EVENT_TYPE::KEY_PRESS == _event_type && "DOWN" == _message) {
			m_position.rotation_angle -= 10;
		}
	}

	Actor(const int _id, const Position _position, const Color _color) :
		m_id(_id), m_position(_position), m_color(_color) {}

	int getId() {
		return Actor::m_id;
	}

	Position getPosition() {
		return Actor::m_position;
	}

	Color getColor() {
		return Actor::m_color;
	}

	void onRender() {
		GameApplicationLayer::drawTriangle(Actor::m_position, Actor::m_color);
	}

	void onUpdate() {
		// m_position.rotation_angle += 1;
	}
};

void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_UP && action == GLFW_PRESS)
		EventManager::getInstance().publishEvent(EVENT_TYPE::KEY_PRESS, "UP");
	else if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
		EventManager::getInstance().publishEvent(EVENT_TYPE::KEY_PRESS, "DOWN");
	else if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
		EventManager::getInstance().publishEvent(EVENT_TYPE::KEY_PRESS, "LEFT");
	else if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
		EventManager::getInstance().publishEvent(EVENT_TYPE::KEY_PRESS, "RIGHT");
}

class KeyListener : public Listener {

};

int main(void) {
	const int SCREEN_WIDTH = 1024;
	const int SCREEN_HEIGHT = 768;
	glewExperimental = true;

	// memory leak debug on
	int tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(tmpDbgFlag);

	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit()) {
		std::cout << "glfw failed" << std::endl;
		return -1;
	}

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hello World", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		std::cout << "window failed" << std::endl;
		return -1;
	}

	glfwSetErrorCallback(error_callback);
	glfwSetKeyCallback(window, key_callback);

	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	/* Initialize the library */
	if (glewInit() != GLEW_OK) {
		std::cout << std::endl << "glew failed" << std::endl;
		return -1;
	}
	
	Actor *triangleActor = new Actor(1, Position(1.0, 1.0, 0.0), Color(1, 1, 1));

	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	std::shared_ptr<Listener> listener_sp(triangleActor);
	std::weak_ptr<Listener> listener_wp(listener_sp);
	EventManager::getInstance().registerListener(EVENT_TYPE::KEY_PRESS, listener_wp);

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-10, 10, 10, -10, 1, 0);

		triangleActor->onUpdate();
		triangleActor->onRender();

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glfwDestroyWindow(window);

	glfwTerminate();
	return 0;
}