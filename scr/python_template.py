import ctypes
import pyautogui
import keyboard
import time
import sys

#  будет заменён на реальный HWN_D
HWND_VALUE = {{HWND_PLACEHOLDER}}

def manage_window(hwnd, show):
    try:
        user32 = ctypes.windll.user32
        user32.ShowWindow(hwnd, 0 if not show else 5)
        if show:
            user32.SetForegroundWindow(hwnd)
    except:
        pass

# Скрываем окно
manage_window(HWND_VALUE, False)

# ========== ПОЛЬЗОВАТЕЛЬСКИЙ КОД ==========
{{USER_CODE_PLACEHOLDER}}
# ==========================================

# Восстанавливаем окно
manage_window(HWND_VALUE, True)