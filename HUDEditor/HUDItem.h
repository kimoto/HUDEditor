#include "Util.h"
#include <Windows.h>
#include <string>
#include <GdiPlus.h>
#include <list>
#pragma comment(lib, "gdiplus.lib")

using namespace std;
using namespace ::Gdiplus;

#define STRETCH_TOP_LEFT 1
#define STRETCH_TOP_RIGHT 2
#define STRETCH_BOTTOM_RIGHT 3
#define STRETCH_BOTTOM_LEFT 4
#define STRETCH_NOT_FOUND 0

// ==================================================================
//	HUDItem
// ==================================================================
class HUDItem
{
public:
	HUDItem(int x, int y, int height, int width, int zOrder);
	~HUDItem();

	// 指定のHDCにこのHUDを描画します
	virtual void draw(HDC);
	virtual void drawDebugInfo(HDC);
	virtual void highlight(HDC);
	virtual BOOL hitTest(POINT pt);
	virtual int hitTestStretchMode(POINT pt);
	virtual HUDItem *getHitTestItem(POINT pt);
	virtual BOOL correctRect(int x, int y, int width, int height, bool flag);
	virtual void relativeMoveTo(int moveX, int moveY);
	virtual void moveTo(int moveX, int moveY);
	virtual void HUDItem::resizeTo(int moveX, int moveY);

	int x;
	int y;
	int zOrder;
	int height;
	int width;
};


// ----------------------------
//	HUDImageItem
// ----------------------------
class HUDImageItem : public HUDItem
{
private:
	Bitmap *image;

public:
	HUDImageItem(int x, int y, int zOrder, wstring filePath) : HUDItem(x,y,zOrder,0,0)
	{
		loadFromFile(filePath);
	}
	
	void loadFromFile(wstring filePath){
		trace(L"load bitmap file: %s\n", filePath.c_str());
		this->image = new Bitmap(filePath.c_str(), FALSE);

		// 画像サイズの大きさに合わせて、要素のサイズ変更
		this->width = this->image->GetWidth();
		this->height = this->image->GetHeight();
		trace(L"imageitem bitmap size: %d,%d\n", this->width, this->height);
	}

	void draw(HDC hdc){
		for(int x=0; x<this->width; x++){
			for(int y=0; y<this->height; y++){
				Color color;
				this->image->GetPixel(x, y, &color);
				::SetPixel(hdc, this->x + x, this->y + y, RGB(color.GetR(),color.GetG(),color.GetB()));
			}
		}

		drawDebugInfo(hdc);
	}
};


// ==================================================================
//	HUDContainer
// ==================================================================
class HUDContainer : public HUDItem
{
public:
	std::list<HUDItem *>hudElements;

	HUDContainer(int x, int y, int w, int h, int z) : HUDItem(x, y, w, h, z) {
		this->x = x;
		this->y = y;
		this->zOrder = z;
		this->width = w;
		this->height = h;
	}
	~HUDContainer(){
		return;
	}

	void draw(HDC);
	BOOL hitTest(POINT pt);
	HUDItem *getHitTestItem(POINT pt);
	BOOL correctRect(int rx, int ry, int rwidth, int rheight, bool flag);
	//void relativeMoveTo(int moveX, int moveY);
	void moveTo(int moveX, int moveY);
};
