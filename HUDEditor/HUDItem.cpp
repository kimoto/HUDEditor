#include "HUDItem.h"

// 汎用的なヒットボックス検証関数
// 指定された短形と点が当たっているかどうか判定する関数
BOOL hitTestRect(int x, int y, int width, int height, POINT pt)
{
	if(x <= pt.x && pt.x <= x + width &&
		y <= pt.y && pt.y <= y + height ){
		return TRUE;
	}
	return FALSE;
}

// 汎用的なヒットボックス検証関数
// 指定された短形と点が当たっているかどうか判定する関数
BOOL hitTestRect(RECT *rect, POINT pt)
{
	return hitTestRect(rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, pt);
}

HUDItem::HUDItem(int x, int y, int height, int width, int zOrder)
{
	this->x = x;
	this->y = y;
	this->zOrder = zOrder;
	this->height = height;
	this->width = width;
}

HUDItem::~HUDItem()
{
}

void HUDItem::draw(HDC hdc)
{
	trace(L"HUDItem::draw();\n");

	::FillRectBrush(hdc, this->x, this->y,
	this->width, this->height, RGB(255,0,0));

	::drawRectColor(hdc, this->x, this->y,
	this->width, this->height, RGB(255,255,255), 1);

	::SetBkMode(hdc, TRANSPARENT);
	::SetTextColor(hdc, RGB(255,255,255));

	drawDebugInfo(hdc);
}

void HUDItem::drawDebugInfo(HDC hdc)
{	
	LPTSTR debugInfo = ::sprintf_alloc(L"%d,%d %d:%d (z:%d)", this->x, this->y, this->width, this->height, this->zOrder);
	::TextOut(hdc, this->x, this->y, debugInfo, lstrlen(debugInfo));
	::GlobalFree(debugInfo);
}

// 四隅に拡大縮小のオブジェクト配置
void HUDItem::highlight(HDC hdc)
{
	int bw = 10; // border width
	COLORREF color = RGB(0, 255, 0);

	// 左上
	::FillRectBrush(hdc, this->x, this->y, bw, bw, color);

	// 右上
	::FillRectBrush(hdc, this->x + this->width - bw, this->y, bw, bw, color);

	// 右下
	::FillRectBrush(hdc, this->x + this->width - bw, this->y + this->height - bw, bw, bw, color);

	// 左下
	::FillRectBrush(hdc, this->x, this->y + this->height - bw, bw, bw, color);
}

BOOL HUDItem::hitTest(POINT pt)
{
	return hitTestRect(this->x, this->y, this->width, this->height, pt);
}

// 拡大縮小用の短形がクリックされたかどうか
// 0 == NOT FOUND
int HUDItem::hitTestStretchMode(POINT pt)
{
	int bw = 10;

	// 短形左上
	if( hitTestRect(this->x, this->y, bw, bw, pt) ){
		trace(L"短径左上が選択されました\n");
		return STRETCH_TOP_LEFT;
	}
	// 短径右上
	if( hitTestRect(this->x + this->width - bw, this->y, width, bw, pt) ){
		trace(L"短径右上が選択されました\n");
		return STRETCH_TOP_RIGHT;
	}
	
	// 右下
	if( hitTestRect(this->x + this->width - bw, this->y + this->height - bw, bw, bw, pt) ){
		trace(L"短形右下が選択されました\n");
		return STRETCH_BOTTOM_RIGHT;
	}

	// 左下
	if( hitTestRect(this->x, this->y + this->height - bw, bw, bw, pt) ){
		trace(L"短形左下が選択されました\n");
		return STRETCH_BOTTOM_LEFT;
	}

	return STRETCH_NOT_FOUND;
}

// ヒットしたらヒットしたHUD要素を返却
// ヒットしなかったらNULLを返却します
HUDItem *HUDItem::getHitTestItem(POINT pt)
{
	if(hitTest(pt)){
		return this;
	}
	return NULL;
}

// 指定の範囲に収める
BOOL HUDItem::correctRect(int rx, int ry, int rwidth, int rheight, bool flag)
{
	BOOL ret = TRUE;

	if(this->x < rx){
		if(flag) this->x = rx;
		ret = FALSE;
	}
	if(this->y < ry){
		if(flag) this->y = ry;
		ret = FALSE;
	}
	if(rwidth < this->x + this->width){
		if(flag) this->x = rwidth - this->width;
		ret = FALSE;
	}
	if(rheight < this->y + this->height){
		if(flag) this->y = rheight - this->height;
		ret = FALSE;
	}
	return ret;
}

void HUDItem::relativeMoveTo(int moveX, int moveY)
{
	this->x += moveX;
	this->y += moveY;
}

void HUDItem::moveTo(int moveX, int moveY)
{
	this->x += moveX;
	this->y += moveY;
}

// 絶対座標で指定の位置にリサイズ
void HUDItem::resizeTo(int w, int h)
{
	this->width += w;
	this->height += h;
}
void HUDContainer::draw(HDC hdc)
{
	trace(L"HUDContainer::draw();\n");

	// 背景自体の描画
	::FillRectBrush(hdc, this->x, this->y, this->width, this->height, RGB(0,0,255));

	// 下位のHUD要素を描画する
	list<HUDItem *>::iterator it = this->hudElements.begin();
	while(it != this->hudElements.end()){
		(*it)->draw(hdc);
		it++;
	}
}

BOOL HUDContainer::hitTest(POINT pt)
{
	// 下位のHUD要素を描画する
	list<HUDItem *>::iterator it = this->hudElements.begin();
	while(it != this->hudElements.end()){
		if( (*it)->hitTest(pt) ){
			return TRUE;
		}
		it++;
	}

	// 背景自体のhitTest
	if(HUDItem::hitTest(pt)){
		return TRUE;
	}
	return FALSE;
}

HUDItem *HUDContainer::getHitTestItem(POINT pt)
{
	// 子要素のヒット判定を先に行う
	// 下位のHUD要素を描画する
	list<HUDItem *>::iterator it = this->hudElements.begin();
	while(it != this->hudElements.end()){
		if( (*it)->hitTest(pt) ){
			return *it; // ヒットしたものを返却する
		}
		it++;
	}

	// 背景自体のhitTest
	if(HUDItem::hitTest(pt)){
		return this;
	}
	return NULL;
}

BOOL HUDContainer::correctRect(int rx, int ry, int rwidth, int rheight, bool flag)
{
	int xx,yy,ww,hh;

	xx = this->x;
	yy = this->y;
	ww = this->width;
	hh = this->height;

	HUDItem::correctRect(rx, ry, rwidth, rheight, flag);
	
	// 親が補正によって移動した分を探す
	xx = xx - this->x;
	yy = yy - this->y;
	ww = ww - this->width;
	hh = hh - this->height;
	trace(L"correct: %d,%d,%d,%d\n", xx, yy, ww, hh);

	// 親が補正された分だけ子供も補正する
	list<HUDItem *>::iterator it = this->hudElements.begin();
	while(it != this->hudElements.end()){
		(*it)->x -= xx;
		(*it)->y -= yy;
		it++;
	}
	
	return TRUE;
}

// 単位は絶対座標
void HUDContainer::moveTo(int moveX, int moveY)
{
	trace(L"HUDContainer move to %d,%d\n", moveX, moveY);
	HUDItem::moveTo(moveX, moveY);

	list<HUDItem *>::iterator it = this->hudElements.begin();
	while(it != this->hudElements.end()){
		(*it)->moveTo(moveX, moveY);
		it++;
	}
}
