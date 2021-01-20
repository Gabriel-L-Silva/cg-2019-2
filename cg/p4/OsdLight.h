
class OsdLight{
public:
	struct Lighting {
		struct Light {
			float position[4];
			float direction[4];
			float ambient[4];
			float diffuse[4];
			float specular[4];
			float color[4];
			float props[4];
		} lightSource[10];
	};
};
