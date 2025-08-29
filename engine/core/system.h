#pragma once

namespace Ecs {

	struct BaseSystem {
		virtual void Init() {}
		virtual void Frame() {}
		virtual void PhysicsFrame() {}
	};

} // namespace Ecs