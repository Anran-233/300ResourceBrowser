#pragma once
#ifndef IMG_DECODE_H
#define IMG_DECODE_H
#include <fstream>
#include <iostream>
class CImgDecede {
private:
	bool m_bDelete{ false };
	const unsigned char* m_src{ nullptr };
	size_t m_src_len{ 0 };
	unsigned char* m_data{ nullptr };
	size_t m_size{ 0 };
	int m_w{ 0 };
	int m_h{ 0 };
	int m_fmt;
    int m_comp{ 0 };
public:
	enum { DDS = 1, TGA = 2 };
	CImgDecede(const char *filename) : m_fmt(0) { 
		std::ifstream in;
		in.open(filename, std::ios::binary);
		if (!in.is_open()) return;
		in.seekg(0, std::ios::end);
		m_src_len = (size_t)in.tellg();
		m_src = new unsigned char[m_src_len];
		m_bDelete = (bool)m_src;
		in.seekg(0, std::ios::beg);
		in.read((char*)m_src, m_src_len);
		in.close();
		decede(); 
	}
	CImgDecede(const void *src, const size_t& size, int fmt = 0) : m_src((const unsigned char*)src), m_src_len(size), m_fmt(fmt) { decede(); }
	~CImgDecede() { if (m_bDelete && m_src) delete[](unsigned char*)m_src; if (m_data) delete[] m_data; }
	inline const unsigned char* data() const { return m_data; }
	inline const size_t& size() const { return m_size; }
	inline const int& w() const { return m_w; }
	inline const int& h() const { return m_h; }
private:
	struct stbi__context {
		unsigned char *img_buffer, *img_buffer_end;
		unsigned char *img_buffer_original, *img_buffer_original_end;
        const unsigned char zero = 0;
		stbi__context(unsigned char *src, const size_t& len)
			: img_buffer(src), img_buffer_end(src + len), 
			img_buffer_original(src), img_buffer_original_end(src + len) {}
        int rewind() { img_buffer = img_buffer_original, img_buffer_end = img_buffer_original_end; return 0; }
		bool copy(unsigned char* buffer, const int& n) {
			if (img_buffer + n >= img_buffer_end) return false;
			memcpy(buffer, img_buffer, n);
			img_buffer += n;
			return buffer;
		}
		unsigned char* getn(const int& n) {
			if (img_buffer + n >= img_buffer_end) return nullptr;
			unsigned char* buffer = img_buffer;
			img_buffer += n;
			return buffer;
		}
        const unsigned char& get8() { if (img_buffer < img_buffer_end) return *img_buffer++; return zero; }
		int get16() {
			const unsigned char z = get8();
			return ((int)get8() << 8) + z;
		}
		void skip(const int& n) {
			if (n < 0) img_buffer = img_buffer_end;
			else img_buffer += n;
		}
	};
	struct DDS_header {
		unsigned int    dwMagic;
		unsigned int    dwSize;
		unsigned int    dwFlags;
		unsigned int    dwHeight;
		unsigned int    dwWidth;
		unsigned int    dwPitchOrLinearSize;
		unsigned int    dwDepth;
		unsigned int    dwMipMapCount;
		unsigned int    dwReserved1[11];
		/*  DDPIXELFORMAT	*/
		struct {
			unsigned int    dwSize;
			unsigned int    dwFlags;
			unsigned int    dwFourCC;
			unsigned int    dwRGBBitCount;
			unsigned int    dwRBitMask;
			unsigned int    dwGBitMask;
			unsigned int    dwBBitMask;
			unsigned int    dwAlphaBitMask;
		} sPixelFormat;
		/*  DDCAPS2	*/
		struct {
			unsigned int    dwCaps1;
			unsigned int    dwCaps2;
			unsigned int    dwDDSX;
			unsigned int    dwReserved;
		} sCaps;
		unsigned int    dwReserved2;
	};
	void decede() {
		stbi__context s((unsigned char*)m_src, m_src_len);
		if (m_fmt == DDS) stbi__dds_info(&s);
        else if (m_fmt == TGA) stbi__tga_info(&s);
		else if (stbi__dds_info(&s)) m_fmt = DDS;
        else if (stbi__tga_info(&s)) m_fmt = TGA;
		if (!m_comp) return;
		s.rewind();
		// 开始解码
		if (m_fmt == DDS) stbi__dds_load(&s);
		else stbi__tga_load(&s);
		if (!m_data) return;
	}
	int stbi__dds_info(stbi__context *s) {
		int flags, is_compressed, has_alpha;
		if (sizeof(DDS_header) != 128) return 0;
		DDS_header* header = (DDS_header*)s->getn(128);
		if (!header) return s->rewind();
		if (header->dwMagic != (('D' << 0) | ('D' << 8) | ('S' << 16) | (' ' << 24))) return s->rewind();
		if (header->dwSize != 124) return s->rewind();
		flags = 0x00000001 | 0x00000002 | 0x00000004 | 0x00001000;
        if ((header->dwFlags & flags) != (unsigned int)flags) return s->rewind();
		if (header->sPixelFormat.dwSize != 32) return s->rewind();
		flags = 0x00000004 | 0x00000040;
		if ((header->sPixelFormat.dwFlags & flags) == 0) return s->rewind();
		if ((header->sCaps.dwCaps1 & 0x00001000) == 0) return s->rewind();
		is_compressed = (header->sPixelFormat.dwFlags & 0x00000004) / 0x00000004;
		has_alpha = (header->sPixelFormat.dwFlags & 0x00000001) / 0x00000001;
		m_w = header->dwWidth;
		m_h = header->dwHeight;
		if (is_compressed) m_comp = 4;
		else if (has_alpha) m_comp = 4;
		else m_comp = 3;
		return 1;
	}
	void stbi__dds_load(stbi__context *s) {
		int DXT_family;
		int has_alpha, has_mipmap;
		int is_compressed, cubemap_faces;
		int block_pitch, num_blocks;
		DDS_header* header = (DDS_header*)s->getn(128);
		if (!header) return;
		is_compressed = (header->sPixelFormat.dwFlags & 0x00000004) / 0x00000004;
		has_alpha = (header->sPixelFormat.dwFlags & 0x00000001) / 0x00000001;
		has_mipmap = (header->sCaps.dwCaps1 & 0x00400000) && (header->dwMipMapCount > 1);
		cubemap_faces = (header->sCaps.dwCaps2 & 0x00000200) / 0x00000200;
		cubemap_faces &= (m_w == m_h);
		cubemap_faces *= 5;
		cubemap_faces += 1;
		block_pitch = (m_w + 3) >> 2;
		num_blocks = block_pitch * ((m_w + 3) >> 2);
		if (is_compressed) {
			/*	compressed	*/
			m_comp = 4;
			DXT_family = 1 + (header->sPixelFormat.dwFourCC >> 24) - '1';
			if ((DXT_family < 1) || (DXT_family > 5)) return;
			m_size = m_w * m_h * 4 * cubemap_faces;
			m_data = new unsigned char[m_size];
			unsigned char nulls[8]{ 0 };
			unsigned char block[16 * 4];
			unsigned char* compressed = nulls;
			for (int cf = 0; cf < cubemap_faces; ++cf) {
				for (int i = 0; i < num_blocks; ++i) {
					int bx, by, bw = 4, bh = 4;
					int ref_x = 4 * (i % block_pitch);
					int ref_y = 4 * (i / block_pitch);
					if (DXT_family == 1) {	//	DXT1
						if (!(compressed = s->getn(8))) compressed = nulls;
						stbi_decode_DXT1_block(block, compressed);
					}
					else if (DXT_family < 4) {	//	DXT2/3
						if (!(compressed = s->getn(8))) compressed = nulls;
						unsigned int next_bit = 0;
						for (unsigned int pos = 3; pos < 16 * 4; pos += 4) {
							auto&& bit = ((compressed[next_bit >> 3] >> (next_bit & 7U)) & 15U) * 255U + 8U;
							block[pos] = (bit + (bit >> 4)) >> 4;
							next_bit += 4;
						}
						if (!(compressed = s->getn(8))) compressed = nulls;
						stbi_decode_DXT_color_block(block, compressed);
					}
					else {	//	DXT4/5
						if (!(compressed = s->getn(8))) compressed = nulls;
						stbi_decode_DXT45_alpha_block(block, compressed);
						if (!(compressed = s->getn(8))) compressed = nulls;
						stbi_decode_DXT_color_block(block, compressed);
					}
					if (ref_x + 4 > m_w) bw = m_w - ref_x;
					if (ref_y + 4 > m_h) bh = m_h - ref_y;
					for (by = 0; by < bh; ++by) {
						int idx = 4 * ((ref_y + by + cf * m_w) * m_w + ref_x);
						for (bx = 0; bx < bw * 4; ++bx) m_data[idx + bx] = block[by * 16 + bx];
					}
				}
				if (has_mipmap) {
					int block_size = 16;
					if (DXT_family == 1) block_size = 8;
					for (int i = 1; i < (int)header->dwMipMapCount; ++i) {
						int mx = m_w >> (i + 2);
						int my = m_h >> (i + 2);
						if (mx < 1) mx = 1;
						if (my < 1) my = 1;
						s->skip(mx * my * block_size);
					}
				}
			}
		}
		else {
			/*	uncompressed	*/
			DXT_family = 0;
			m_size = m_w * m_h * m_comp * cubemap_faces;
			m_data = new unsigned char[m_size];
			for (int cf = 0; cf < cubemap_faces; ++cf) {
				s->copy(&m_data[cf * m_w * m_h * m_comp], m_w * m_h * m_comp);
				if (has_mipmap) {
					for (int i = 1; i < (int)header->dwMipMapCount; ++i) {
						int mx = m_w >> i;
						int my = m_h >> i;
						if (mx < 1) mx = 1;
						if (my < 1) my = 1;
						s->skip(mx * my * m_comp);
					}
				}
			}
			for (size_t i = 0; i < m_size; i += m_comp) std::swap(m_data[i], m_data[i + 2]);
		}
		m_h *= cubemap_faces;
		has_alpha = 0;
		if (m_comp == 4) {
			for (size_t i = 3; (i < m_size) && (has_alpha == 0); i += 4) has_alpha |= (m_data[i] < 255);
		}
		else {
			stbi__convert_format(4);
			m_comp = 4;
		}
	}
	void stbi_rgb_888_from_565(unsigned int c, unsigned char& r, unsigned char& g, unsigned char& b) {
		auto&& r_bit = ((c >> 11) & 0b00011111U) * 255U + 16U;
		r = (r_bit + (r_bit >> 5)) >> 5;
		auto&& g_bit = ((c >> 05) & 0b00111111U) * 255U + 32U;
		g = (g_bit + (g_bit >> 6)) >> 6;
		auto&& b_bit = ((c >> 00) & 0b00011111U) * 255U + 16U;
		b = (b_bit + (b_bit >> 5)) >> 5;
	}
	void stbi__convert_format(const int& req_comp) {
		if (req_comp == m_comp) return;
		unsigned char *good{ nullptr };
		if (m_w > 0 && m_w < 20000 && m_h > 0 && m_h < 20000) good = new unsigned char[m_w * m_h * req_comp];
		if (!good) {
			delete[] m_data;
			m_data = nullptr;
			m_size = 0; 
			return;
		}
#define STBI__COMBO(a,b)  ((a)*8+(b))
#define STBI__CASE(i,a,b)   case STBI__COMBO(a,b): for (i = m_w - 1; i >= 0; --i, src += a, dest += b)
		auto lam_compute_y = [](const int& r, const int& g, const int& b) -> unsigned char { return (unsigned char)(((r * 77) + (g * 150) + (29 * b)) >> 8); };
		int i, j;
		for (j = 0; j < m_h; ++j) {
			unsigned char *src = m_data + j * m_w * m_comp;
			unsigned char *dest = good + j * m_w * req_comp;
			switch (STBI__COMBO(m_comp, req_comp)) {
				STBI__CASE(i, 1, 2) { dest[0] = src[0], dest[1] = 255; } break;
				STBI__CASE(i, 1, 3) { dest[0] = dest[1] = dest[2] = src[0]; } break;
				STBI__CASE(i, 1, 4) { dest[0] = dest[1] = dest[2] = src[0], dest[3] = 255; } break;
				STBI__CASE(i, 2, 1) { dest[0] = src[0]; } break;
				STBI__CASE(i, 2, 3) { dest[0] = dest[1] = dest[2] = src[0]; } break;
				STBI__CASE(i, 2, 4) { dest[0] = dest[1] = dest[2] = src[0], dest[3] = src[1]; } break;
				STBI__CASE(i, 3, 4) { dest[0] = src[0], dest[1] = src[1], dest[2] = src[2], dest[3] = 255; } break;
				STBI__CASE(i, 3, 1) { dest[0] = lam_compute_y(src[0], src[1], src[2]); } break;
				STBI__CASE(i, 3, 2) { dest[0] = lam_compute_y(src[0], src[1], src[2]), dest[1] = 255; } break;
				STBI__CASE(i, 4, 1) { dest[0] = lam_compute_y(src[0], src[1], src[2]); } break;
				STBI__CASE(i, 4, 2) { dest[0] = lam_compute_y(src[0], src[1], src[2]), dest[1] = src[3]; } break;
				STBI__CASE(i, 4, 3) { dest[0] = src[0], dest[1] = src[1], dest[2] = src[2]; } break;
			default: break;
			}
#undef STBI__COMBO
#undef STBI__CASE
		}
		std::swap(m_data, good);
		delete[] good;
		return;
	}
	void stbi_decode_DXT_color_block(unsigned char uncompressed[16 * 4], unsigned char compressed[8]) {
		unsigned char decode_colors[4 * 3];
		//	find the 2 primary colors
		unsigned int&& c0 = compressed[0] + (compressed[1] << 8);
		unsigned int&& c1 = compressed[2] + (compressed[3] << 8);
		stbi_rgb_888_from_565(c0, decode_colors[0], decode_colors[1], decode_colors[2]);
		stbi_rgb_888_from_565(c1, decode_colors[3], decode_colors[4], decode_colors[5]);
		//	Like DXT1, but no choicees:
		//	no alpha, 2 interpolated colors
		decode_colors[6] = (2 * decode_colors[0] + decode_colors[3]) / 3;
		decode_colors[7] = (2 * decode_colors[1] + decode_colors[4]) / 3;
		decode_colors[8] = (2 * decode_colors[2] + decode_colors[5]) / 3;
		decode_colors[9] = (decode_colors[0] + 2 * decode_colors[3]) / 3;
		decode_colors[10] = (decode_colors[1] + 2 * decode_colors[4]) / 3;
		decode_colors[11] = (decode_colors[2] + 2 * decode_colors[5]) / 3;
		//	decode the block
		int next_bit = 4 * 8;
		for (int i = 0; i < 16 * 4; i += 4) {
			int idx = ((compressed[next_bit >> 3] >> (next_bit & 7)) & 3) * 3;
			next_bit += 2;
			uncompressed[i + 0] = decode_colors[idx + 0];
			uncompressed[i + 1] = decode_colors[idx + 1];
			uncompressed[i + 2] = decode_colors[idx + 2];
		}
	}
	void stbi_decode_DXT1_block(unsigned char uncompressed[16 * 4], unsigned char compressed[8]) {
		unsigned char decode_colors[4 * 4];
		//	find the 2 primary colors
		unsigned int&& c0 = compressed[0] + (compressed[1] << 8);
		unsigned int&& c1 = compressed[2] + (compressed[3] << 8);
		stbi_rgb_888_from_565(c0, decode_colors[0], decode_colors[1], decode_colors[2]);
		decode_colors[3] = 255;
		stbi_rgb_888_from_565(c1, decode_colors[4], decode_colors[5], decode_colors[6]);
		decode_colors[7] = 255;
		if (c0 > c1) {
			//	no alpha, 2 interpolated colors
			decode_colors[8] = (2 * decode_colors[0] + decode_colors[4]) / 3;
			decode_colors[9] = (2 * decode_colors[1] + decode_colors[5]) / 3;
			decode_colors[10] = (2 * decode_colors[2] + decode_colors[6]) / 3;
			decode_colors[11] = 255;
			decode_colors[12] = (decode_colors[0] + 2 * decode_colors[4]) / 3;
			decode_colors[13] = (decode_colors[1] + 2 * decode_colors[5]) / 3;
			decode_colors[14] = (decode_colors[2] + 2 * decode_colors[6]) / 3;
			decode_colors[15] = 255;
		}
		else {
			//	1 interpolated color, alpha
			decode_colors[8] = (decode_colors[0] + decode_colors[4]) / 2;
			decode_colors[9] = (decode_colors[1] + decode_colors[5]) / 2;
			decode_colors[10] = (decode_colors[2] + decode_colors[6]) / 2;
			decode_colors[11] = 255;
			decode_colors[12] = 0;
			decode_colors[13] = 0;
			decode_colors[14] = 0;
			decode_colors[15] = 0;
		}
		//	decode the block
		int next_bit = 4 * 8;
		for (int i = 0; i < 16 * 4; i += 4) {
			int idx = ((compressed[next_bit >> 3] >> (next_bit & 7)) & 3) * 4;
			next_bit += 2;
			uncompressed[i + 0] = decode_colors[idx + 0];
			uncompressed[i + 1] = decode_colors[idx + 1];
			uncompressed[i + 2] = decode_colors[idx + 2];
			uncompressed[i + 3] = decode_colors[idx + 3];
		}
	}
	void stbi_decode_DXT45_alpha_block(unsigned char uncompressed[16 * 4], unsigned char compressed[8]) {
		int next_bit = 8 * 2;
		unsigned char decode_alpha[8];
		//	each alpha value gets 3 bits, and the 1st 2 bytes are the range
		decode_alpha[0] = compressed[0];
		decode_alpha[1] = compressed[1];
		if (decode_alpha[0] > decode_alpha[1]) {
			//	6 step intermediate
			decode_alpha[2] = (6 * decode_alpha[0] + 1 * decode_alpha[1]) / 7;
			decode_alpha[3] = (5 * decode_alpha[0] + 2 * decode_alpha[1]) / 7;
			decode_alpha[4] = (4 * decode_alpha[0] + 3 * decode_alpha[1]) / 7;
			decode_alpha[5] = (3 * decode_alpha[0] + 4 * decode_alpha[1]) / 7;
			decode_alpha[6] = (2 * decode_alpha[0] + 5 * decode_alpha[1]) / 7;
			decode_alpha[7] = (1 * decode_alpha[0] + 6 * decode_alpha[1]) / 7;
		}
		else {
			//	4 step intermediate, pluss full and none
			decode_alpha[2] = (4 * decode_alpha[0] + 1 * decode_alpha[1]) / 5;
			decode_alpha[3] = (3 * decode_alpha[0] + 2 * decode_alpha[1]) / 5;
			decode_alpha[4] = (2 * decode_alpha[0] + 3 * decode_alpha[1]) / 5;
			decode_alpha[5] = (1 * decode_alpha[0] + 4 * decode_alpha[1]) / 5;
			decode_alpha[6] = 0;
			decode_alpha[7] = 255;
		}
		for (int i = 3; i < 16 * 4; i += 4) {
			int idx = 0, bit;
			bit = (compressed[next_bit >> 3] >> (next_bit & 7)) & 1;
			idx += bit << 0;
			++next_bit;
			bit = (compressed[next_bit >> 3] >> (next_bit & 7)) & 1;
			idx += bit << 1;
			++next_bit;
			bit = (compressed[next_bit >> 3] >> (next_bit & 7)) & 1;
			idx += bit << 2;
			++next_bit;
			uncompressed[i] = decode_alpha[idx & 7];
		}
	}
	int stbi__tga_info(stbi__context *s) {
		unsigned char tga_colormap_bpp;
		s->get8();                   // discard Offset
		const auto& tga_colormap_type = s->get8(); // colormap type
		if (tga_colormap_type > 1) return s->rewind();      // only RGB or indexed allowed
		const auto& tga_image_type = s->get8(); // image type
		if (tga_colormap_type == 1) { // colormapped (paletted) image
			if (tga_image_type != 1 && tga_image_type != 9) return s->rewind();
			s->skip(4);       // skip index of first colormap entry and number of entries
			const auto& sz = s->get8();    //   check bits per palette color entry
			if ((sz != 8) && (sz != 15) && (sz != 16) && (sz != 24) && (sz != 32)) return s->rewind();
			s->skip(4);       // skip image x and y origin
			tga_colormap_bpp = sz;
		}
		else { // "normal" image w/o colormap - only RGB or grey allowed, +/- RLE
			if ((tga_image_type != 2) && (tga_image_type != 3) && (tga_image_type != 10) && (tga_image_type != 11)) return s->rewind();
			s->skip(9); // skip colormap specification and image x/y origin
			tga_colormap_bpp = 0;
		}
		const auto& tga_w = s->get16();
		if (tga_w < 1) return s->rewind();
		const auto& tga_h = s->get16();
		if (tga_h < 1) return s->rewind();
		const auto& tga_bits_per_pixel = s->get8(); // bits per pixel
		s->get8(); // ignore alpha bits
		int tga_comp;
		if (tga_colormap_bpp != 0) {
			if ((tga_bits_per_pixel != 8) && (tga_bits_per_pixel != 16)) return s->rewind();
			tga_comp = stbi__tga_get_comp(tga_colormap_bpp, false, nullptr);
		}
		else tga_comp = stbi__tga_get_comp(tga_bits_per_pixel, (tga_image_type == 3) || (tga_image_type == 11), nullptr);
		if (!tga_comp) return s->rewind();
		m_w = tga_w;
		m_h = tga_h;
		m_comp = tga_comp;
		return 1;                   // seems to have passed everything
	}
	int stbi__tga_get_comp(const unsigned char& bits_per_pixel, const bool& is_grey, bool* is_rgb16) {
		if (is_rgb16) *is_rgb16 = false;
		switch (bits_per_pixel) {
		case 8:  return 1;
		case 16: if (is_grey) return 2; else return 0;	// else: fall-through
		case 15: if (is_rgb16) *is_rgb16 = true; return 3;
		case 24: return 0;	// fall-through
		case 32: return 4;
		default: return 0;
		}
	}
	void stbi__tga_load(stbi__context *s) {
		int&& tga_offset = (int)s->get8();			// 图像信息长度
		auto& tga_indexed = s->get8();				// 是否启用颜色索引表: 0 不启用; 1 启用
		int&& tga_image_type = (int)s->get8();		// 图像类型 低2位: 0 无; 1 索引色; 2 真彩色; 3 黑白色; 
		int&& tga_is_RLE = tga_image_type & 0b1000;	// 第4位: 0 未压缩; 1 RLE压缩;
		tga_image_type &= 0b0111;
		const int& tga_palette_start = s->get16();	// 颜色索引表起始位
		const int& tga_palette_len = s->get16();	// 颜色索引表长度
		auto& tga_palette_bits = s->get8();			// 色彩位数
		s->skip(8);									// 跳过部分图像描述
		auto& tga_bits_per_pixel = s->get8();		// 像素占用字节位数
		const bool& tga_inverted = !(s->get8() & 0b00100000);	//   do a tiny bit of precessing

		// 确认色彩位数
		bool tga_rgb16{ false };
		if (tga_indexed) m_comp = stbi__tga_get_comp(tga_palette_bits, false, &tga_rgb16);
		else m_comp = stbi__tga_get_comp(tga_bits_per_pixel, (tga_image_type == 3), &tga_rgb16);
		if (!m_comp) return;

		s->skip(tga_offset);	// 跳过图像信息
		if (m_w <= 0 || m_w >= 20000 || m_h <= 0 || m_h >= 20000) return;
		m_size = m_w * m_h * m_comp;
		if (!(m_data = new unsigned char[m_size])) {
			m_size = 0;
			return;
		}
		if (!tga_indexed && !tga_is_RLE && !tga_rgb16) {	// 无索引色、无压缩、非rgb16
			for (int i = 0; i < m_h; ++i) {
				int row = tga_inverted ? m_h - i - 1 : i;
				unsigned char *tga_row = m_data + row * m_w * m_comp;
				s->copy(tga_row, m_w * m_comp);
			}
		}
		else {
			unsigned char *tga_palette{ nullptr };
			if (tga_indexed) {	// 读取索引表
				s->skip(tga_palette_start);
				if (tga_rgb16) {
					if (!(tga_palette = new unsigned char[tga_palette_len * m_comp])) {
						m_size = 0;
						delete[] m_data;
						return;
					}
					unsigned char *pal_entry = tga_palette;
					for (int i = 0; i < tga_palette_len; ++i) {
						const auto& px = s->get16();
						pal_entry[0] = (unsigned char)((((px >> 10) & 31U) * 255U) / 31U);
						pal_entry[1] = (unsigned char)((((px >> 05) & 31U) * 255U) / 31U);
						pal_entry[2] = (unsigned char)((((px >> 00) & 31U) * 255U) / 31U);
						pal_entry += m_comp;
					}
				}
				else if (!(tga_palette = s->getn(tga_palette_len * m_comp))) {
					m_size = 0;
					delete[] m_data;
					return;
				}
			}
			int RLE_count = 0;
			int RLE_repeating = 0;
			int read_next_pixel = 1;
			for (int i = 0; i < m_w * m_h; ++i) {
				if (tga_is_RLE) {	// RLE解压
					if (RLE_count == 0) {
						const auto& RLE_cmd = s->get8();
						RLE_count = 1 + (RLE_cmd & 127U);
						RLE_repeating = RLE_cmd >> 7;
						read_next_pixel = 1;
					}
					else if (!RLE_repeating) read_next_pixel = 1;
				}
				else read_next_pixel = 1;
				unsigned char* raw_data = &m_data[i * m_comp];
				if (read_next_pixel) {
					if (tga_indexed) {
						int pal_idx = (tga_bits_per_pixel == 8) ? (int)s->get8() : s->get16();
						if (pal_idx >= tga_palette_len) pal_idx = 0;
						pal_idx *= m_comp;
						for (int j = 0; j < m_comp; ++j) raw_data[j] = tga_palette[pal_idx + j];
					}
					else if (tga_rgb16) {
						const auto& px = s->get16();
						raw_data[0] = (unsigned char)((((px >> 10) & 31U) * 255U) / 31U);
						raw_data[1] = (unsigned char)((((px >> 05) & 31U) * 255U) / 31U);
						raw_data[2] = (unsigned char)((((px >> 00) & 31U) * 255U) / 31U);
					}
					else {
						for (int j = 0; j < m_comp; ++j) raw_data[j] = s->get8();
					}
					read_next_pixel = 0;
				}
				--RLE_count;
			}
			if (tga_inverted) {
				for (int j = 0; j * 2 < m_h; ++j) {
					int index1 = j * m_w * m_comp;
					int index2 = (m_h - 1 - j) * m_w * m_comp;
					for (int i = m_w * m_comp; i > 0; --i) {
						unsigned char temp = m_data[index1];
						m_data[index1] = m_data[index2];
						m_data[index2] = temp;
						++index1;
						++index2;
					}
				}
			}
			if (tga_rgb16) delete[] tga_palette;
		}
		// swap RGB - if the source data was RGB16, it already is in the right order
		if (m_comp >= 3 && !tga_rgb16) {
			unsigned char* tga_pixel = m_data;
			for (int i = 0; i < m_w * m_h; ++i) {
				std::swap(tga_pixel[0], tga_pixel[2]);
				tga_pixel += m_comp;
			}
		}
		// to RGBA8888
		if (4 != m_comp) stbi__convert_format(4);
	}
};
#endif // IMG_DECODE_H
