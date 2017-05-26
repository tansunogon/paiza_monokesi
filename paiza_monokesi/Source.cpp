#include <iostream>
#include <memory>
#include <queue>
#include <immintrin.h>
#include <assert.h>

#include <random>

using namespace std;

struct iostream_init_struct
{ 
	iostream_init_struct()
	{
		std::cin.tie(0);
		std::ios::sync_with_stdio(false);
	}
} iostream_init;

const int WIDTH = 50;
const int HEIGHT = 50;
const int BEAM_WIDTH = 100;
const int NUM_COLORS = 3;

enum Color
{
	RED = 0,
	BLUE = 1,
	GREEN = 2,
	NONE,
};

char toChar(Color color)
{
	switch (color)
	{
	case RED:
		return 'R';
	case BLUE:
		return 'B';
	case GREEN:
		return 'G';
	default:
		return '1';
	}
}

Color toColor(char color)
{
	switch (color)
	{
	case 'R':
		return Color::RED;
	case 'B':
		return Color::BLUE;
	case 'G':
		return Color::GREEN;
	default:
		assert(false);
		return Color::NONE;
	}
}

class Point
{
public:
	Point(int x, int y) : x(x), y(y) {}
	Point(int x, int y, std::shared_ptr<Point>& parent)
		: x(x), y(y), parent(parent) {}

	int x; // 0, 1, 2, ...
	int y; // 1, 2, 3, ...
	std::shared_ptr<Point> parent;

	void print() const
	{
		cout << "x: " << x
		     << " y: " << y;
	}

	void printAnswer(int H) const
	{
		std::vector<Point> path;
		path.push_back(*this);

		std::shared_ptr<Point> p = this->parent;
		while (p)
		{
			path.push_back(*p);
			p = p->parent;
		}

		cout << path.size() << endl;
		for (auto itr = path.rbegin(); itr != path.rend(); ++itr)
		{
			cout << (itr->x + 1) << ' ' << (H - itr->y + 1) << endl;
			// cout << itr->x << ' ' << itr->y << endl;
		}
	}
};

class EraseInfo;
class EraseInfoContainer;

class OneColorBits
{
public:
	OneColorBits() : m_(0) {}

	static OneColorBits makeOneBit(int y)
	{
		assert(y >= 1 && y <= HEIGHT);
		return OneColorBits(1ll << y);
	}

	void set(int y)
	{
		assert(y >= 1 && y <= HEIGHT);
		m_ |= 1ll << y;
	}

	void set(OneColorBits x)
	{
		m_ |= x.m_;
	}

	bool get(int y) const
	{
		assert(y >= 1 && y <= HEIGHT);
		return m_ & (1ll << y);
	}

	inline int64_t getBits() const
	{
		return m_;
	}

	void erase(OneColorBits eraseBits)
	{
		m_ &= ~eraseBits.m_;
	}

	// this            = 0011100110
	// neighboringBits = 0101000010
	// returns         = 0011100110
	OneColorBits getConnecting(OneColorBits neighboringBits) const
	{
		int64_t leading_bits = getLeadingBits(m_, neighboringBits.m_);
		int64_t trailing_bits = getTrailingBits(m_, neighboringBits.m_);
		return OneColorBits(leading_bits - (trailing_bits << 1));
	}

	inline OneColorBits getOneBit() const
	{
		assert(m_);
		return OneColorBits(m_ & -m_);
	}

	inline int tzcnt() const
	{
		assert(m_);
#ifdef __GNUC__
		return __builtin_ctzll(m_);
#elif _M_X64
		return _tzcnt_u64(m_);
#else
		return popcount((m_ & -m_) - 1);
#endif
	}

	void drop(OneColorBits eraseBits)
	{
		m_ = pext(m_, ~eraseBits.m_);
	}

	int countBits() const
	{
		return popcount(m_);
	}

	void print(Color color) const
	{
		char ch = toChar(color);
		for (int i = 1; i <= HEIGHT; ++i)
		{
			if (m_ & (1ll << i))
				cout << ch;
			else
				cout << ' ';
		}
		cout << endl;
	}
private:
	OneColorBits(int64_t m) : m_(m) {}

	int64_t m_;
private:
	/* ------------------ static bit operation functions ------------------ */

	// 01010 => 01111
	inline static int64_t fill_trailing_bits(int64_t a)
	{
		a |= a >> 1;
		a |= a >> 2;
		a |= a >> 4;
		a |= a >> 8;
		a |= a >> 16;
		a |= a >> 32;
		return a;
	}

	// 0001010 => 0101000
	inline static int64_t reverse_bits(int64_t x)
	{
		x = ((x & 0x5555555555555555) <<  1) | ((x >>  1) & 0x5555555555555555);
		x = ((x & 0x3333333333333333) <<  2) | ((x >>  2) & 0x3333333333333333);
		x = ((x & 0x0f0f0f0f0f0f0f0f) <<  4) | ((x >>  4) & 0x0f0f0f0f0f0f0f0f);
		x = ((x & 0x00ff00ff00ff00ff) <<  8) | ((x >>  8) & 0x00ff00ff00ff00ff);
		x = ((x & 0x0000ffff0000ffff) << 16) | ((x >> 16) & 0x0000ffff0000ffff);
		x = ((x & 0x00000000ffffffff) << 32) | ((x >> 32) & 0x00000000ffffffff);
		return x;
	}

	inline static int popcount(int64_t x)
	{
#ifdef __GNUC__
		return __builtin_popcountll(x);
#elif _M_X64
		return (int)__popcnt64(x);
#else
		x = (x & 0x5555555555555555) + ((x >>  1) & 0x5555555555555555);
		x = (x & 0x3333333333333333) + ((x >>  2) & 0x3333333333333333);
		x = (x & 0x0f0f0f0f0f0f0f0f) + ((x >>  4) & 0x0f0f0f0f0f0f0f0f);
		x = (x & 0x00ff00ff00ff00ff) + ((x >>  8) & 0x00ff00ff00ff00ff);
		x = (x & 0x0000ffff0000ffff) + ((x >> 16) & 0x0000ffff0000ffff);
		x = (x & 0x00000000ffffffff) + ((x >> 32) & 0x00000000ffffffff);
		return (int)x;
#endif
	}

	// m_ = 01110
	// x  = 00100
	// =>   10000
	inline static int64_t getLeadingBits(int64_t m, int64_t x)
	{
		return (m + (x & m)) & ~m;
	}

	// m_ = 01110
	// x  = 00100
	// =>   00001
	inline static int64_t getTrailingBits(int64_t m, int64_t x)
	{
		int64_t rev_m = reverse_bits(m);
		int64_t rev_x = reverse_bits(x);
		return reverse_bits(getLeadingBits(rev_m, rev_x));
	}

	// Paralell bit extract
	//inline static int64_t pext(int64_t source, int64_t mask)
	//{
	//	int64_t ret = 0;
	//	for (uint64_t copy_dst = 1; mask;)
	//	{
	//		// example:
	//		// source = abcdef
	//		// mask   = 110011
	//		// ret    = **abef
	//		//
	//		// low_bit    = 000001
	//		// high_bit   = 110100
	//		// diff       = 000111
	//		// chunk_mask = 000011
	//		int64_t low_bit = mask & -mask;
	//		int64_t high_bit = mask + low_bit;
	//		int64_t diff = high_bit ^ mask;
	//		int64_t chunk_mask = diff & mask;
	//		int64_t ext_data = source & chunk_mask;

	//		ret |= ext_data * copy_dst / low_bit;

	//		// copied    = 000100
	//		// next mask = 110000
	//		int64_t copied = (diff ^ chunk_mask) / low_bit;
	//		copy_dst *= copied;
	//		mask ^= chunk_mask;
	//	}
	//	return ret;
	//}

	// Paralell bit extract
	inline static int64_t pext(int64_t source, int64_t mask)
	{
#ifdef __BMI2__
		return _pext_u64(source, mask);
#else
		int64_t ret = 0;
		int copy_dst = 1;
		// for (int i = 0; i < 64; ++i)
		for (int i = 1; i <= HEIGHT; ++i)
		{
			if (mask & (1ll << i))
			{
				ret |= (1ll & (source >> i)) << copy_dst;
				++copy_dst;
			}
		}
		return ret;
#endif
	}
};

class ThreeColorField;

class OneColorField
{
public:
	OneColorField() {}

	void set(int x, int y)
	{
		assert(x >= 0 && x < WIDTH);
		assert(y >= 1 && y <= HEIGHT);
		
		line[x].set(y);
	}

	bool get(int x, int y) const
	{
		assert(x >= 0 && x < WIDTH);
		assert(y >= 1 && y <= HEIGHT);
		
		return line[x].get(y);
	}

	void drop(const OneColorField& fieldToErase)
	{
		for (int i = 0; i < WIDTH; ++i)
		{
			line[i].drop(fieldToErase.line[i]);
		}
	}

	bool empty() const
	{
		for (int i = 0; i < WIDTH; ++i)
		{
			if (line[i].getBits())
			{
				return false;
			}
		}
		return true;
	}

	void eraseAllChunk(EraseInfoContainer& container,
	                   Color color,
	                   int score,
	                   std::shared_ptr<Point>& parentPoint,
	                   const ThreeColorField& originalField);
	int eraseAllChunk_sub(int i, OneColorBits eraseBits,
	                      OneColorField& eraseField);

	// void eraseChunk(const EraseInfo& eraseInfo);
	

	void print(Color color) const
	{
		for (int i = 0; i < WIDTH; ++i)
		{
			line[i].print(color);
		}
		cout << "------------------------------------" << endl;
	}
private:
	OneColorBits line[WIDTH];
};

class ThreeColorField
{
public:
	ThreeColorField() {}
	ThreeColorField(bool random)
	{
		if (true)
		{
			std::random_device rnd;
			for (int x = 0; x < WIDTH; ++x)
			{
				for (int y = 1; y <= HEIGHT; ++y)
				{
					int color = rnd() % NUM_COLORS;
					field[color].set(x, y);
				}
			}
		}
	}

	void set(int x, int y, Color color)
	{
		field[color].set(x, y);
	}

	void tryEraseAllChunk(EraseInfoContainer& container,
	                      int score,
	                      std::shared_ptr<Point> parentPoint) const
	{
		ThreeColorField temp = *this;
		temp.eraseAllChunk(container, score, parentPoint, temp);
	}

	void eraseAllChunk(EraseInfoContainer& container,
	                   int score,
	                   std::shared_ptr<Point>& parentPoint,
	                   ThreeColorField originalField)
	{
		for (int i = 0; i < NUM_COLORS; ++i)
		{
			field[i].eraseAllChunk(container,
			                       Color(i),
			                       score,
			                       parentPoint,
			                       originalField);
		}
	}

	void drop(const OneColorField& fieldToErase)
	{
		for (int i = 0; i < NUM_COLORS; ++i)
		{
			field[i].drop(fieldToErase);
		}
	}

	
	// void eraseChunk(const EraseInfo& eraseInfo);

	void print() const
	{
		for (int x = 0; x < WIDTH; ++x)
		{
			for (int y = 1; y <= HEIGHT; ++y)
			{
				if (field[Color::RED].get(x, y))
				{
					cout << 'R';
				}
				else if (field[Color::BLUE].get(x, y))
				{
					cout << 'B';
				}
				else if (field[Color::GREEN].get(x, y))
				{
					cout << 'G';
				}
				else
				{
					cout << ' ';
				}
			}
			cout << endl;
		}
		cout << "--------------------------------------------------" << endl;
	}

	void printSeparated() const
	{
		field[0].print(Color::RED);
		field[1].print(Color::BLUE);
		field[2].print(Color::GREEN);
		cout << "--------------------------------------------------" << endl;
	}
private:
	OneColorField field[NUM_COLORS];
};

class EraseInfo
{
public:
	std::shared_ptr<Point> point;
	Color color;
	int score; // num of erased blocks

	OneColorField erase_field;
	ThreeColorField dropped_field;

	EraseInfo(int x,
	          int y,
	          Color color,
	          int score,
	          OneColorField&& erase_field,
	          std::shared_ptr<Point>& parent,
	          const ThreeColorField& originalField)
	{
		point = make_shared<Point>(x, y, parent);
		this->color = color;
		this->score = score;
		this->erase_field = erase_field;
		this->dropped_field = originalField;
	}

	//void setDroppedField(const ThreeColorField& field)
	//{
	//	dropped_field = field;
	//}

	bool operator<(const EraseInfo& rhs) const
	{
		return this->score > rhs.score;
	}

	void print() const
	{
		cout << "x: " << point->x
		     << " y: " << point->y
		     << " color: " << toChar(color)
		     << " score: " << score << endl;
	}
};

class EraseInfoContainer
{
public:
	EraseInfoContainer()
	{
		container.reserve(BEAM_WIDTH);
	}

	bool shouldPush(int score)
	{
		if (container.size() < BEAM_WIDTH)
		{
			return true;
		}
		else
		{
			return score > container.front().score;
		}
	}

	void push(EraseInfo&& x)
	{
		if (container.size() == BEAM_WIDTH - 1)
		{
			container.emplace_back(x);
			std::make_heap(container.begin(), container.end());
		}
		else if (container.size() < BEAM_WIDTH)
		{
			container.emplace_back(x);
		}
		else
		{
			assert(x.score > container.front().score);
			pop_heap(container.begin(), container.end());
			container.pop_back();

			container.push_back(x);
			push_heap(container.begin(), container.end());
		}
	}

	const EraseInfo& fetchBestEraseInfo() const
	{
		assert(container.size() >= 1);
		int max_score = 0;
		int max_index = 0;
		for (int i = 0; i < container.size(); ++i)
		{
			if (container[i].score > max_score)
			{
				max_score = container[i].score;
				max_index = i;
			}
		}

		return container[max_index];
	}

	void clear()
	{
		container.clear();
	}

	std::vector<EraseInfo>& getVector()
	{
		return container;
	}

	void swap(EraseInfoContainer& rhs)
	{
		container.swap(rhs.container);
	}
private:
	std::vector<EraseInfo> container;
};

void OneColorField::eraseAllChunk(EraseInfoContainer& container,
                                  Color color,
                                  int score,
                                  std::shared_ptr<Point>& parentPoint,
                                  const ThreeColorField& originalField)
{
	for (int i = 0; i < WIDTH; ++i)
	{
		while (line[i].getBits())
		{
			OneColorBits ignition_point = line[i].getOneBit();
			OneColorField eraseField;
			int num_of_erased = eraseAllChunk_sub(i, ignition_point, eraseField);
			int new_score = score + num_of_erased;
			if (container.shouldPush(new_score))
			{
				container.push(
					EraseInfo(i, // x
					ignition_point.tzcnt(), // y
					color,
					new_score, //score
					move(eraseField),
					parentPoint,
					originalField));
			}

			//info.print();
			//print(color);
		}
	}
}

// erase blocks which are connected to eraseBits
// param i: line index
// returns num of erased blocks
int OneColorField::eraseAllChunk_sub(int i, OneColorBits eraseBits,
                                     OneColorField& eraseField)
{
	if (i < 0 || i >= WIDTH)
	{
		return 0;
	}

	OneColorBits connectingBits = line[i].getConnecting(eraseBits);
	if (connectingBits.getBits())
	{
		eraseField.line[i].set(connectingBits);

		line[i].erase(connectingBits);
		return connectingBits.countBits()
		       + eraseAllChunk_sub(i - 1, connectingBits, eraseField)
		       + eraseAllChunk_sub(i + 1, connectingBits, eraseField);
	}
	else
	{
		return 0;
	}
}

//void OneColorField::eraseChunk(const EraseInfo& eraseInfo)
//{
//	for (int i = 0; i < WIDTH; ++i)
//	{
//		line[i].erase(eraseInfo.erase_field.line[i]);
//	}
//}

//void ThreeColorField::eraseChunk(const EraseInfo& eraseInfo)
//{
//	field[eraseInfo.color].eraseChunk(eraseInfo);
//}


EraseInfo beam_search(ThreeColorField& field, int N, int H, int maxScore)
{
	EraseInfoContainer con;
	EraseInfoContainer con2;
	bool finished = false;

	field.tryEraseAllChunk(con, 0, shared_ptr<Point>());
	for (auto& eraseInfo : con.getVector())
	{
		eraseInfo.dropped_field = field;
		eraseInfo.dropped_field.drop(eraseInfo.erase_field);
	}

	for (int i = 0; i < N - 1 && !finished; ++i)
	{
		con2.clear();

		for (auto& eraseInfo : con.getVector())
		{
			eraseInfo.dropped_field.tryEraseAllChunk(con2,
			                                         eraseInfo.score,
			                                         eraseInfo.point);
		}

		for (auto& eraseInfo : con2.getVector())
		{
			eraseInfo.dropped_field.drop(eraseInfo.erase_field);
			if (eraseInfo.score == maxScore)
			{
				finished = true;
			}
		}

		con.swap(con2);
	}

	return con.fetchBestEraseInfo();
}

int main()
{
#if 0
	int W, H, N;
	cin >> W >> H >> N;
	ThreeColorField field;
	for (int y = H; y >= 1; --y)
	{
		for (int x = 0; x < W; ++x)
		{
			char c;
			cin >> c;
			field.set(x, y, toColor(c));
		}
	}

	EraseInfo info = beam_search(field, 100, H, W * H);
#else
	ThreeColorField field(true);
	EraseInfo info = beam_search(field, 100, HEIGHT, WIDTH * HEIGHT);
#endif
	info.print();
	info.point->printAnswer(HEIGHT);
	info.dropped_field.print();
}