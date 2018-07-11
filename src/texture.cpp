#include "texture.h"

// TODO Use polymorphism instead of duplicated class

// <sternmull> you could create the key like this: std::ostringstream s;  s << "@" << static_cast<void*>(address); std::string key = s.str();
// <sternmull> and have a Texture::init(std::string key) that is used by both constructors.
std::unordered_map<std::string, std::shared_ptr<vita2d_texture>> Texture::textureCache1;
std::unordered_map<unsigned char *, std::shared_ptr<vita2d_texture>> Texture::textureCache2;


void DeleteTexture(vita2d_texture* tex)
{
	dbg_printf(DBG_DEBUG, "Destroying texture...");
	vita2d_free_texture(tex);
}


Texture::Texture(const Texture& that)
{
	texture = that.texture;
	caching_ = that.caching_;
}

Texture& Texture::operator=(const Texture& that)
{
    if (this != &that)
    {
		texture = that.texture;
		caching_ = that.caching_;
    }
    return *this;
}

Texture::Texture(const std::string &path, bool caching) :
	caching_(caching)
{
    auto key = path;
	if (caching_) {
		std::lock_guard<std::mutex> lock(mtx1_);

		if (textureCache1.count(key) >= 1)
		{
			texture = textureCache1[key];
			return;
		}
	}

	std::size_t found = path.find_last_of(".");
	std::string extension = path.substr(found+1);

	texture = std::make_shared(vita2d_load_PNG_file(path.c_str()));
	if (!texture) texture = std::make_shared(vita2d_load_JPEG_file(path.c_str()));
	if (!texture) texture = std::make_shared(vita2d_load_BMP_file(path.c_str()));

	if (!texture) {
		dbg_printf(DBG_ERROR, "Couldn't load texture %s", path.c_str());
		return;
	}


	if (caching_) {
		std::lock_guard<std::mutex> lock(mtx1_);
		textureCache1[key] = texture;
	}
}

Texture::Texture(unsigned char* addr, bool caching) : caching_(caching)
{
	//dbg_printf(DBG_DEBUG, "Looking for size %d, path: %s", fSize, path.c_str());


	auto key = addr;
	if (caching) {
		std::lock_guard<std::mutex> lock(mtx2_);
		if (textureCache2.count(key) >= 1) {
			texture = textureCache2[key];
			return;
		}
	}

	texture = std::make_shared(vita2d_load_PNG_buffer(addr));

	if (caching) {
		std::lock_guard<std::mutex> lock(mtx2_);
		textureCache2[key] = texture;
	}
}


int Texture::Draw(const Point &pt)
{
	vita2d_draw_texture(texture.get(), pt.x, pt.y);
	return 0;
}


int Texture::DrawExt(const Point &pt, int alpha)
{
	vita2d_draw_texture_tint(texture.get(), pt.x, pt.y, RGBA8(255, 255, 255, alpha));
	return 0;
}


// vita2d doesn't have a draw resize function: https://github.com/xerpi/libvita2d/issues/42
int Texture::DrawResize(const Point &pt1, const Point &dimensions)
{
	// Careful, vita2d_texture_get_width() will crash if vita2d_start_drawing() was not called
	float width = vita2d_texture_get_width(texture.get());
	float height = vita2d_texture_get_height(texture.get());
	if (width == 0 || height == 0) {
		vita2d_draw_texture(texture.get(), pt1.x, pt1.y);
		return 0;
	}


    float stretchX = ((float)dimensions.x) / width;
    float stretchY = ((float)dimensions.y) / height;
    vita2d_draw_texture_scale(texture.get(), pt1.x, pt1.y, stretchX, stretchY);

    return 0;
}

int Texture::DrawTint(const Point &pt, unsigned int color)
{
	vita2d_draw_texture_tint(texture.get(), pt.x, pt.y, color);
	return 0;
}

