#include "SceneNavigationCamera.hpp"

#include "core/Time.hpp"
#include "renderer/Camera.hpp"
#include "renderer/scene/Entity.hpp"
#include "renderer/scene/Scene.hpp"

#include "glm/gtx/string_cast.hpp"
#include "editor/ImGuiExtension.hpp"

namespace Core {
	void SceneNavigationCamera::Awake()
	{
		enabled = false;
	}

	void SceneNavigationCamera::Start()
	{
		transform = GetTransform();
		nativeCamera = entity->GetComponent<CameraComponent>()->camera();

		glm::vec3 dir = anchorPoint - transform->position();
		anchorDir = normalize(dir);
		radius = glm::length(dir) / glm::length(anchorDir);
	}

	void SceneNavigationCamera::Update()
	{
		if (!enabled)
			return;

		HandleMovement();
	}

	void SceneNavigationCamera::Shutdown()
	{
	}

	void SceneNavigationCamera::HandleMovement()
	{
		// moving with mouse
		if (auto mouseDeltaRaw = Input::Get()->GetMouseDelta(); 
			mouseDeltaRaw != glm::vec2())
		{
			glm::vec2 mouseDelta = Time::DeltaTime * mouseDeltaRaw;

			// moves anchor point
			if (NativeInput::IsMouseButtonPressed(anchoredKey) && NativeInput::IsKeyPressed(Key::LeftShift))
			{
				glm::vec3 offset = anchoredMoveSensitivity * (mouseDelta.x * transform->Left() + mouseDelta.y * transform->Up());
				anchorPoint += offset;
				transform->SetPosition(transform->position() + offset);
				return;
			}

			//anchored rotation at (0,0)
			if (NativeInput::IsMouseButtonPressed(anchoredKey))
			{
				transform->RotateAround(anchorPoint + anchorOffset, mouseDelta.y * anchoredRotationSensitivity, transform->Left());
				transform->RotateAround(anchorPoint + anchorOffset, mouseDelta.x * anchoredRotationSensitivity, Vec3::Back);
				anchorDir = normalize(anchorPoint - transform->position());
				return;
			}
		}
			
		// moving with keyboard -- with acceleration
		if (NativeInput::IsMouseButtonPressed(anchoredMoveKeyboard)) 
		{
			if (auto keyboardInput = NativeInput::GetVec3Input(planeNavKeyBindings);
				keyboardInput != Vec3::Zero)
			{
				glm::vec3 move = moveSpeed * Time::DeltaTime * keyboardInput;

				auto offset = move.x * transform->Right() + move.z * Vec3::Forward;

				anchorPoint += offset;
					
				radius += move.y * zoomSpeed;
				if (radius < minimumRadius) 
					radius = minimumRadius;

				transform->SetPosition(anchorPoint - radius * anchorDir + offset);
				return;
			}
		}
	}

	/////////////////////////////////////////////////////
	// Event Handling
	/////////////////////////////////////////////////////

	void SceneNavigationCamera::HandleEvent(Event& e)
	{
		if (!enabled)
			return;

		e.Dispatch<MouseScrolledEvent>(NE_BIND_EVENT_FN(SceneNavigationCamera::OnMouseScroll));
	}

	void SceneNavigationCamera::Inspect()
	{
		static std::string name = "Script Name: " + std::string(getClassName());
		ImGui::Text(name.c_str());
		ImGuiExt::ColumnDragFloat("Fly Speed", "###FlySpeed", &moveSpeed, 0.05f, 0.0f, FLT_MAX, "%.1f", ImGuiSliderFlags_AlwaysClamp);
		ImGuiExt::ColumnDragFloat("Zoom Speed", "###ZS", &zoomSpeed, 0.01f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
		ImGuiExt::ColumnDragFloat("Orbital Rotation Speed", "###ORS", &anchoredRotationSensitivity, 0.1f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
		ImGuiExt::ColumnDragFloat("Orbital Move Speed", "###OMS", &anchoredMoveSensitivity, 0.01f, 0.0f, FLT_MAX, "%.2f", ImGuiSliderFlags_AlwaysClamp);
	}

	bool SceneNavigationCamera::OnMouseScroll(MouseScrolledEvent& e)
	{
		auto offset = e.m_YOffset * zoomSpeed;

		if (nativeCamera->orthographic) {
			nativeCamera->orthographicScale -= offset;
			if (nativeCamera->orthographicScale < 0.001f) 
				nativeCamera->orthographicScale = 0.001f;
		}
		else {
			radius -= offset;
			if (radius < minimumRadius) 
				radius = minimumRadius;
			transform->SetPosition(anchorPoint - radius * anchorDir);
		}
		return false;
	}
}

template<>
void Scene::OnComponentAdded<Core::SceneNavigationCamera>(Entity& entity, Core::SceneNavigationCamera& component)
{
	this->OnComponentAdded<Behaviour>(entity, component);
}

template<>
void Scene::OnComponentRemoved<Core::SceneNavigationCamera>(Entity& entity, Core::SceneNavigationCamera& component)
{
	this->OnComponentRemoved<Behaviour>(entity, component);
}