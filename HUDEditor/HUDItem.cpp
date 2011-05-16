#include "HUDItem.h"

// �ėp�I�ȃq�b�g�{�b�N�X���؊֐�
// �w�肳�ꂽ�Z�`�Ɠ_���������Ă��邩�ǂ������肷��֐�
BOOL hitTestRect(int x, int y, int width, int height, POINT pt)
{
	if(x <= pt.x && pt.x <= x + width &&
		y <= pt.y && pt.y <= y + height ){
		return TRUE;
	}
	return FALSE;
}

// �ėp�I�ȃq�b�g�{�b�N�X���؊֐�
// �w�肳�ꂽ�Z�`�Ɠ_���������Ă��邩�ǂ������肷��֐�
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

// �l���Ɋg��k���̃I�u�W�F�N�g�z�u
void HUDItem::highlight(HDC hdc)
{
	int bw = 10; // border width
	COLORREF color = RGB(0, 255, 0);

	// ����
	::FillRectBrush(hdc, this->x, this->y, bw, bw, color);

	// �E��
	::FillRectBrush(hdc, this->x + this->width - bw, this->y, bw, bw, color);

	// �E��
	::FillRectBrush(hdc, this->x + this->width - bw, this->y + this->height - bw, bw, bw, color);

	// ����
	::FillRectBrush(hdc, this->x, this->y + this->height - bw, bw, bw, color);
}

BOOL HUDItem::hitTest(POINT pt)
{
	return hitTestRect(this->x, this->y, this->width, this->height, pt);
}

// �g��k���p�̒Z�`���N���b�N���ꂽ���ǂ���
// 0 == NOT FOUND
int HUDItem::hitTestStretchMode(POINT pt)
{
	int bw = 10;

	// �Z�`����
	if( hitTestRect(this->x, this->y, bw, bw, pt) ){
		trace(L"�Z�a���オ�I������܂���\n");
		return STRETCH_TOP_LEFT;
	}
	// �Z�a�E��
	if( hitTestRect(this->x + this->width - bw, this->y, width, bw, pt) ){
		trace(L"�Z�a�E�オ�I������܂���\n");
		return STRETCH_TOP_RIGHT;
	}
	
	// �E��
	if( hitTestRect(this->x + this->width - bw, this->y + this->height - bw, bw, bw, pt) ){
		trace(L"�Z�`�E�����I������܂���\n");
		return STRETCH_BOTTOM_RIGHT;
	}

	// ����
	if( hitTestRect(this->x, this->y + this->height - bw, bw, bw, pt) ){
		trace(L"�Z�`�������I������܂���\n");
		return STRETCH_BOTTOM_LEFT;
	}

	return STRETCH_NOT_FOUND;
}

// �q�b�g������q�b�g����HUD�v�f��ԋp
// �q�b�g���Ȃ�������NULL��ԋp���܂�
HUDItem *HUDItem::getHitTestItem(POINT pt)
{
	if(hitTest(pt)){
		return this;
	}
	return NULL;
}

// �w��͈̔͂Ɏ��߂�
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

// ��΍��W�Ŏw��̈ʒu�Ƀ��T�C�Y
void HUDItem::resizeTo(int w, int h)
{
	this->width += w;
	this->height += h;
}
void HUDContainer::draw(HDC hdc)
{
	trace(L"HUDContainer::draw();\n");

	// �w�i���̂̕`��
	::FillRectBrush(hdc, this->x, this->y, this->width, this->height, RGB(0,0,255));

	// ���ʂ�HUD�v�f��`�悷��
	list<HUDItem *>::iterator it = this->hudElements.begin();
	while(it != this->hudElements.end()){
		(*it)->draw(hdc);
		it++;
	}
}

BOOL HUDContainer::hitTest(POINT pt)
{
	// ���ʂ�HUD�v�f��`�悷��
	list<HUDItem *>::iterator it = this->hudElements.begin();
	while(it != this->hudElements.end()){
		if( (*it)->hitTest(pt) ){
			return TRUE;
		}
		it++;
	}

	// �w�i���̂�hitTest
	if(HUDItem::hitTest(pt)){
		return TRUE;
	}
	return FALSE;
}

HUDItem *HUDContainer::getHitTestItem(POINT pt)
{
	// �q�v�f�̃q�b�g������ɍs��
	// ���ʂ�HUD�v�f��`�悷��
	list<HUDItem *>::iterator it = this->hudElements.begin();
	while(it != this->hudElements.end()){
		if( (*it)->hitTest(pt) ){
			return *it; // �q�b�g�������̂�ԋp����
		}
		it++;
	}

	// �w�i���̂�hitTest
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
	
	// �e���␳�ɂ���Ĉړ���������T��
	xx = xx - this->x;
	yy = yy - this->y;
	ww = ww - this->width;
	hh = hh - this->height;
	trace(L"correct: %d,%d,%d,%d\n", xx, yy, ww, hh);

	// �e���␳���ꂽ�������q�����␳����
	list<HUDItem *>::iterator it = this->hudElements.begin();
	while(it != this->hudElements.end()){
		(*it)->x -= xx;
		(*it)->y -= yy;
		it++;
	}
	
	return TRUE;
}

// �P�ʂ͐�΍��W
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
