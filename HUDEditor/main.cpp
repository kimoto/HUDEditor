//	�\������
//	HUDContainer > HUDItem
//	HUDContainer > HUDContainer�Ƃ͂Ȃ�Ȃ�
//
#include <Windows.h>
#include <WindowsX.h>
#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <time.h>
#include "Util.h"
#include <GdiPlus.h>
#pragma comment(lib, "gdiplus.lib")
#include <list>
using namespace std;
using namespace ::Gdiplus;

#define MUTEX_NAME L"L4D2 HUD Editor"
#define IDI_MAIN 0
HINSTANCE g_hInstance = NULL;
HWND g_hWnd = NULL;
TCHAR szWindowClass[] = L"L4D2 HUD Editor";
TCHAR szTitle[] = L"L4D2 HUD Editor";

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

class DoubleBuffering
{
private:
	HDC firstDC;
	HDC secondDC;
	HBITMAP secondHBitmap;

public:
	int width, height;

	void createSecondBuffer(HDC original, int width, int height);

	void setFirstBuffer(HDC hdc);
	void setSecondBuffer(HDC hdc);

	void getSecondBuffer();
	void getFirstBuffer();

	// �������ݐ�p�̃������o�b�t�@�ւ̃A�h���X���擾����
	HDC getBuffer();

	void swap(); // �o�b�t�@����ւ�
};

void DoubleBuffering::createSecondBuffer(HDC original, int width, int height)
{
	// original HDC�����ɁA���̃o�b�t�@�𐶐����܂�
	this->firstDC = original;
	this->width = width;
	this->height = height;
	this->secondDC = ::CreateCompatibleDC(original);
	this->secondHBitmap = ::CreateCompatibleBitmap(original, width, height);
	::SelectObject(this->secondDC, this->secondHBitmap);
}

HDC DoubleBuffering::getBuffer()
{
	return this->secondDC;
}

void DoubleBuffering::swap()
{
	::BitBlt(this->firstDC, 0, 0, this->width, this->height, this->secondDC, 0, 0, SRCCOPY);
}

// ==================================================================
//	HUDItem
// ==================================================================
class HUDItem
{
public:
	HUDItem(int x, int y, int height, int width, int zOrder);
	~HUDItem();

	// �w���HDC�ɂ���HUD��`�悵�܂�
	virtual void draw(HDC);
	virtual void drawDebugInfo(HDC);
	virtual BOOL hitTest(POINT pt);
	virtual HUDItem *getHitTestItem(POINT pt);
	virtual BOOL correctRect(int x, int y, int width, int height, bool flag);
	virtual void relativeMoveTo(int moveX, int moveY);
	virtual void moveTo(int moveX, int moveY);

	int x;
	int y;
	int zOrder;
	int height;
	int width;
};

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

BOOL HUDItem::hitTest(POINT pt)
{
	if(this->x <= pt.x && pt.x <= this->x + this->width &&
		this->y <= pt.y && pt.y <= this->y + this->height){
		return TRUE;
	}
	return FALSE;
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
	this->x = moveX;
	this->y = moveY;
}

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

		// �摜�T�C�Y�̑傫���ɍ��킹�āA�v�f�̃T�C�Y�ύX
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

	// �O��ʒu����̍��������߂�
	// ���̍��Ԃ񂾂��A�R���e�i���̃I�u�W�F�N�g���ړ�������
	int diff_x = moveX - this->x;
	int diff_y = moveY - this->y;

	// �������g�̈ړ����s��
	this->x += diff_x;
	this->y += diff_y;

	list<HUDItem *>::iterator it = this->hudElements.begin();
	while(it != this->hudElements.end()){
		(*it)->x += diff_x;
		(*it)->y += diff_y;
		it++;
	}
}

//list<HUDItem *> hudItemList;
HUDContainer *container = new HUDContainer(0, 0, 400, 400, 0);
list<HUDItem *>::iterator it;

BOOL bDrag = FALSE;
HUDItem *activeItem = NULL; // ���݈ړ�����item�ւ̃|�C���^
POINT mousePressedPt = {0}; // �}�E�X�������Ƃ��̍��W
POINT mouseReleasedPt = {0}; // �}�E�X�������Ƃ��̍��W
POINT mousePressedActiveItemPt = {0};

// �w�i�摜�̓ǂݍ���
ULONG_PTR token;
GdiplusStartupInput input;
Bitmap *backgroundBitmap;
DoubleBuffering buffering;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static ULONG_PTR token;

	switch(message) {
	case WM_CREATE:
		{
			GdiplusStartup(&token, &input, NULL);
			backgroundBitmap = new Bitmap(L"background.jpg", FALSE);
			trace(L"bitmap size: %d,%d\n", backgroundBitmap->GetWidth(), backgroundBitmap->GetHeight());

			// �w�i�摜�̑傫���ɍ��킹�ăN���C�A���g�̈�g��
			// �N���C�A���g�̈�̃T�C�Y�ƁA�E�C���h�E�̈�̃T�C�Y�������Z����
			// ���j���[�Ƃ��̑傫�����ׂ邻�̍� + �{���̗v�f�̍���
			RECT windowRect, clientRect;
			::GetWindowRect(hWnd, &windowRect);
			::GetClientRect(hWnd, &clientRect);

			int diffHeight = (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top);
			int diffWidth = (windowRect.right - windowRect.left) - (clientRect.right - clientRect.left);
			trace(L"diff height: %d, diff width: %d\n", diffHeight, diffWidth);
			::MoveWindow(hWnd, windowRect.left, windowRect.top, backgroundBitmap->GetWidth() + diffWidth, backgroundBitmap->GetHeight() + diffHeight, TRUE);

			//HUDContainer *c = new HUDContainer(0, 0, 600, 600, 0);
			container->hudElements.push_back(new HUDItem(0, 0, 100, 100, 0));
			container->hudElements.push_back(new HUDItem(500, 500, 100, 100, 1));
			container->hudElements.push_back(new HUDItem(300, 500, 100, 100, 2));
			container->hudElements.push_back(new HUDImageItem(400,400,3, L"hudelement.png"));
			
			// ������DC�̍쐬
			buffering.createSecondBuffer(::GetDC(hWnd), backgroundBitmap->GetWidth(), backgroundBitmap->GetHeight());
			
			// �w�i�摜�ǂݍ���
			for(unsigned int x=0; x<backgroundBitmap->GetWidth(); x++){
				for(unsigned int y=0; y<backgroundBitmap->GetHeight(); y++){
					Color color;
					if(backgroundBitmap->GetPixel(x, y, &color) != ::Gdiplus::Ok){
						trace(L"err\n");
					}

					short r = color.GetR();
					short g = color.GetG();
					short b = color.GetB();

					::SetPixel(buffering.getBuffer(), x, y, RGB(r,g,b));
				}
			}
		}
		break;
	case WM_LBUTTONDOWN:
		{
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			POINT pt = {x, y};

			bool hitFlag = false;

			HUDItem *item = NULL;
			if( container->hitTest(pt) ){
				item = container->getHitTestItem(pt);
			}
			
			// �q�b�g���Ă邩�ǂ���
			if(HUDItem *item = container->getHitTestItem(pt)){
				::trace(L"mouseL Hit!!\n");
				
				// �q�b�g���Ă���ΏۂƂȂ�HUD�v�f���L��
				//::activeItem = *it;
				::activeItem = item;
				mousePressedPt.x = pt.x;
				mousePressedPt.y = pt.y;

				// �}�E�X�N���b�N���ꂽ�Ƃ���activeitem�̍��W
				mousePressedActiveItemPt.x = item->x;
				mousePressedActiveItemPt.y = item->y;

				bDrag = TRUE;
				::SetCapture(hWnd);
			}else{
				::trace(L"mouseL Not Hit!!\n");
				bDrag = FALSE;
				::ReleaseCapture();
			}
		}
		break;
	case WM_MOUSEMOVE:
		{
			if(bDrag){
				int x = GET_X_LPARAM(lParam);
				int y = GET_Y_LPARAM(lParam);
				::trace(L"dragging: %d,%d now\n", x, y);
				
				// �I�u�W�F�N�g����ʊO�ɏo�Ă���␳���Ē��ɖ߂�
				// ��΍��W�ňړ������܂�
				// ���݂̃}�E�X�̍��W - �h���b�O�J�n�������W
				// ����ɂ��h���b�O�J�n�����Ƃ�����̈ړ��������킩��
				// correct���ꂽ���ʂƂ̍����̂ق�������
				activeItem->moveTo(mousePressedActiveItemPt.x + x - mousePressedPt.x,
					mousePressedActiveItemPt.y + y - mousePressedPt.y);
				activeItem->correctRect(0, 0, buffering.width, buffering.height, true);
				
				::trace(L"dragging: -> %d,%d now\n", activeItem->x, activeItem->y);
			
				// ��ʂ��X�V
				::InvalidateRect(hWnd, NULL, FALSE);
			}
		}
		break;
	case WM_LBUTTONUP:
		// �h���b�O���I������Ƃ�
		if(bDrag){
			bDrag = FALSE;
			
			// �h���b�M���O��Ԃ�����
			::ReleaseCapture();
			trace(L"drag exit\n");
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = ::BeginPaint(hWnd, &ps);

			// ���o�b�t�@����\��swap
			buffering.swap();

			// �e�I�u�W�F�N�g�̕`��
			container->draw(hdc);
			
			::EndPaint(hWnd, &ps);
		}
		break;
	case WM_CLOSE:
		::DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		::Gdiplus::GdiplusShutdown(token);
		::PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: �����ɃR�[�h��}�����Ă��������B
	MSG msg;

	// �O���[�o������������������Ă��܂��B
	MyRegisterClass(hInstance);

	// �A�v���P�[�V�����̏����������s���܂�:
	if (!InitInstance (hInstance, nCmdShow)){
		return FALSE;
	}
	
	// ���C�� ���b�Z�[�W ���[�v:
	while (GetMessage(&msg, NULL, 0, 0)){
		if (!TranslateAccelerator(msg.hwnd, NULL, &msg)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	//wcex.style = CS_DBLCLKS;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = ::LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = ::LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	g_hInstance = hInstance; // �O���[�o���ϐ��ɃC���X�^���X�������i�[���܂�

	hWnd = CreateWindowEx(0, szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
	g_hWnd = hWnd;

	if(!hWnd){
		::ShowLastError();
		return FALSE;
	}

	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
	return TRUE;
}

