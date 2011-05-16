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
#include "HUDItem.h"
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


HUDContainer *container = new HUDContainer(0, 0, 400, 400, 0);
list<HUDItem *>::iterator it;

BOOL bDrag = FALSE;
BOOL bStretchMode = FALSE; // �g��k�����[�h
int bStretchPos = 0; // �ǂ̊g��k���̈悪�����ꂽ��
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

			// �q�b�g���Ă邩�ǂ���
			if(HUDItem *item = container->getHitTestItem(pt)){
				::trace(L"mouseL Hit!!\n");
				
				// �q�b�g���Ă���ΏۂƂȂ�HUD�v�f���L��
				::activeItem = item;
				mousePressedPt.x = pt.x;
				mousePressedPt.y = pt.y;

				// �}�E�X�N���b�N���ꂽ�Ƃ���activeitem�̍��W
				mousePressedActiveItemPt.x = item->x;
				mousePressedActiveItemPt.y = item->y;

				// �X�g���b�`���[�h�̒Z�`�ŃN���b�N��������Ă��邩�ǂ���
				if(activeItem->hitTestStretchMode(pt)){
					trace(L"Go Stretch Mode\n");
					bStretchPos = activeItem->hitTestStretchMode(pt);
					::bStretchMode = TRUE;
				}else{
					::bStretchMode = FALSE;
				}

				::InvalidateRect(hWnd, NULL, FALSE);

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
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			::trace(L"dragging: %d,%d now\n", x, y);
	
			if(bDrag && bStretchMode){
				// �g��k�����[�h
				::trace(L"stretch mode now\n");
				::trace(L"***DIFF: %d,%d", x - ::mousePressedPt.x, y - ::mousePressedPt.y);

				if(bStretchPos == STRETCH_BOTTOM_RIGHT){
					// ����Œ�g�����
					activeItem->resizeTo(x - ::mousePressedPt.x, y - ::mousePressedPt.y);
				}
				if(bStretchPos == STRETCH_TOP_RIGHT){
					// �����Œ�g�����
					int diffx = x - ::mousePressedPt.x;
					int diffy = y - ::mousePressedPt.y;

					activeItem->x += 0; // kotei
					activeItem->y += diffy;
					activeItem->width += diffx;
					activeItem->height -= diffy;
				}
				if(bStretchPos == STRETCH_TOP_LEFT){
					// �E���Œ�g�����
					int diffx = x - ::mousePressedPt.x;
					int diffy = y - ::mousePressedPt.y;

					activeItem->x += diffx; // kotei
					activeItem->y += diffy;
					activeItem->width -= diffx;
					activeItem->height -= diffy;
				}
				if(bStretchPos == STRETCH_BOTTOM_LEFT){
					// �E��Œ����
					int diffx = x - ::mousePressedPt.x;
					int diffy = y - ::mousePressedPt.y;

					activeItem->x += diffx; // kotei
					activeItem->y += 0;
					activeItem->width -= diffx;
					activeItem->height += diffy;
				}

				::mousePressedPt.x = x;
				::mousePressedPt.y = y;

				::InvalidateRect(hWnd, NULL, FALSE);

			}else if(bDrag){
				// �I�u�W�F�N�g����ʊO�ɏo�Ă���␳���Ē��ɖ߂�
				// ��΍��W�ňړ������܂�
				// ���݂̃}�E�X�̍��W - �h���b�O�J�n�������W
				// ����ɂ��h���b�O�J�n�����Ƃ�����̈ړ��������킩��
				// correct���ꂽ���ʂƂ̍����̂ق�������
				//activeItem->moveTo(mousePressedActiveItemPt.x + x - mousePressedPt.x,
					//mousePressedActiveItemPt.y + y - mousePressedPt.y);
				activeItem->moveTo(x - mousePressedPt.x, y - mousePressedPt.y);
				activeItem->correctRect(0, 0, buffering.width, buffering.height, true);
				
				::trace(L"dragging: -> %d,%d now\n", activeItem->x, activeItem->y);
			
				::mousePressedPt.x = x;
				::mousePressedPt.y = y;

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
		if(bStretchMode){
			bStretchMode = FALSE;
			bStretchPos = 0;
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

			if(activeItem){
				trace(L"drawing activeItem: %X\n", activeItem);
				activeItem->highlight(hdc);
			}

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

