#pragma once

#include "core/layers/Layer.hpp"

class Editor : public Layer
{
public:
	void OnAttach() override;
	void OnUpdate() override;
	void OnDetach() override;
	void OnEvent(Event& e) override;
	void OnImGuiRender() override;
	void OnViewportRender() override;
};

