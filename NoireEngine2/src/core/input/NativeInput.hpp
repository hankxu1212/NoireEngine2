#include "core/KeyCodes.hpp"
#include "core/MouseCodes.hpp"

#include <glm/glm.hpp>


// wrapper for 4 keys, representing 1D axis mapping
struct InputTypeVec1
{
	KeyCode forward = Key::W;
	KeyCode backward = Key::S;
};

// wrapper for 4 keys, representing 2D axis mapping
struct InputTypeVec2
{
	KeyCode forward = Key::W;
	KeyCode backward = Key::S;
	KeyCode left = Key::A;
	KeyCode right = Key::D;
};

// wrapper for 6 keys, representing 3D axis mapping
struct InputTypeVec3
{
	KeyCode forward = Key::W;
	KeyCode backward = Key::S;
	KeyCode left = Key::A;
	KeyCode right = Key::D;
	KeyCode up = Key::Space;
	KeyCode down = Key::LeftShift;
};

class NativeInput
{
public:
	static bool GetKeyPressed(KeyCode key);

	static bool GetMouseButtonPressed(MouseCode button);

	static bool GetKeyRepeat(KeyCode key);

	static bool GetMouseButtonRepeat(MouseCode button);

	static bool GetKeyRelease(KeyCode key);

	static bool GetMouseButtonRelease(MouseCode button);

	static glm::vec2 GetMousePosition();

	/**
	 * Returns the Vec1(float) output from input key bindings.
	 * Representing a 1D m_Direction.
	 *
	 * \param inputKeys the key bindings (2 in total)
	 * \return the vec1 output, normalized
	 */
	static float GetVec1Input(InputTypeVec1 inputKeys);

	/**
	 * Returns the Vec2 output from input key bindings.
	 * Representing a 2D m_Direction.
	 *
	 * \param inputKeys the key bindings (4 in total)
	 * \return the vec2 output, normalized
	 */
	static glm::vec2 GetVec2Input(InputTypeVec2 inputKeys);

	/**
	 * Returns the Vec3 output from input key bindings.
	 * Representing a 3D m_Direction.
	 *
	 * \param inputKeys the key bindings (6 in total)
	 * \return the vec3 output, normalized
	 */
	static glm::vec3 GetVec3Input(InputTypeVec3& inputKeys);
};

