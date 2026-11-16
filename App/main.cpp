#include <windows.h>
#include <memory>
#include "App.h"

/**
 * Windowsアプリケーションのエントリポイント (WinMain)
 * * 本関数はOSのカーネルによって呼び出され、プロセス空間の初期化後に
 * アプリケーションのインスタンスハンドル(hInstance)を取得する。
 */
int WINAPI WinMain(
    _In_     HINSTANCE hInstance,      // アプリケーションの現在のインスタンスハンドル
    _In_opt_ HINSTANCE hPrevInstance,  // 常にNULL (Win32の歴史的経緯による)
    _In_     LPSTR     lpCmdLine,      // コマンドライン引数
    _In_     int       nCmdShow)       // ウィンドウの表示状態指示
{
    // --- 1. COM (Component Object Model) ライブラリの初期化 ---
    // DirectX 11のリソース管理(ComPtr)や、後続のWIC(画像ロード)、
    // XAudio2(サウンド)の利用を想定し、STA(Single-Threaded Apartment)モデルで初期化を行う。
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return 0;

    // --- 2. アプリケーションクラスのインスタンス化 ---
    // unique_ptrによるスコープ管理を行い、異常終了時も含めた確実なリソース破棄を担保する。
    auto app = std::make_unique<Engine::App>();

    // --- 3. システムおよびハードウェアリソースの初期化 ---
    // ウィンドウ生成、D3D11デバイス生成、パイプラインステートの構築をシーケンシャルに実行。
    // 解像度は標準的な 800x600 に設定。
    if (SUCCEEDED(app->Initialize(hInstance, 1280, 720)))
    {
        // --- 4. メッセージループの開始 ---
        // OSからのメッセージキューを監視し、イベントドリブンなアプリケーション更新を実行。
        // アプリケーションが終了(WM_QUIT)するまで本スレッドの制御権を保持する。
        app->Run();
    }

    // --- 5. COM ライブラリの終了処理 ---
    // unique_ptrのデストラクタによりAppクラスが破棄され、
    // 全てのComPtrがリリースされた後にCOMを終了させる。
    CoUninitialize();

    return 0;
}
