import pyautogui
currentMouse = pyautogui.position()
curPosStr = str(currentMouse)
Xpos = curPosStr[curPosStr.find("x=")+2:curPosStr.find(",")] #получение сырого числа X
Ypos = curPosStr[curPosStr.find("y=")+2:curPosStr.find(")")] #получение сырого числа Y
line = Xpos+","+" "+Ypos
with open('coords.txt', 'w') as file:
    file.write(line)
