import tkinter as tk    # UI 라이브러리
import math
import serial
import time

WIDTH = 640
HEIGHT = 480
angle = 0
direction = 0
objects = [[0,0],[10,0],[20,0],[30,0],[40,0],[50,0],[60,0],[70,110],[80,0],
           [90,0],[100,0],[110,0],[120,0],[130,0],[140,30],[150,0],[160,0],
           [170,150],[180,0]]     # 2차원 리스트

ser = serial.Serial("COM11", 115200)

# 개체 호출
root = tk.Tk()
root.title("Ultrasonic Radar")
canvas = tk.Canvas(root, width=WIDTH, height=HEIGHT, bg="black")
canvas.pack()

def drawObject(angle, distance):
    radius = WIDTH / 2
    x = radius + math.cos(angle * math.pi / 180) * distance
    y = radius - math.sin(angle * math.pi / 180) * distance
    canvas.create_oval(x-5, y-5, x+5, y+5, fill='green') # 사각형 안의 원 그려줌
    

# 함수 생성
def updateScan():
    global angle    # 변수 값을 변경해야 해서 global, 참조만 할거면 없어도됨
    global direction
    global objects
    global sendingAngle
    receiveDistance = 0

    #각도 전송
    if angle % 10 == 0:     # angle==10 일때만 실행(리스트가 angle 10 단위)
        sendingAngle = angle
        mask = b'\x7f'
        ser.write(bytes(bytearray([0x02, 0x52])))
        angleH = (angle >> 7) + 128
        angleL = (angle & mask[0]) + 128   # 0b1000 0000
        crc = (0x02 + 0x52 + angleH + angleL) % 256
        ser.write(bytes(bytearray([angleH, angleL, crc, 0x03])))

    # 거리 수신
    if ser.in_waiting > 0:
        data = ser.read()
        if data == b'\x02':
            # 두번째 바이트 수신대기
            timeout = time.time() + 0.002   # 2ms
            lostData = False
            while ser.in_waiting < 5:    # 5글자 대기
                # 타임아웃 처리
                if time.time() > timeout:   # 2ms 넘으면 lost data
                    lostData = True
                    break
            if lostData == False:
                data = ser.read(5)  # CMD ~ ETX (STX는 위에서 검사 했음)
                if data[0] == 65:
                    # CRC 검사
                    crc = (2 + data[0] + data[1] + data[2]) % 256
                    if crc == data[3]:
                        if data[4] == 3:    # ETX 검사
                            # 데이터 파싱
                            mask = b'\x7f'
                            data_one = bytes([data[1] & mask[0]])
                            receiveDistance = int.from_bytes(data_one) << 7
                            data_one = bytes([data[2] & mask[0]])
                            receiveDistance += int.from_bytes(data_one)
                            # 물체 업데이트
                            for obj in objects:
                                if obj[0] == sendingAngle:
                                    obj[1] = receiveDistance
            
    # 화면 지우기
    canvas.delete('all')
    
    # 레이더 선 그리기
    radius = WIDTH / 2     # 기준점
    length = radius
    x = radius + math.cos(angle * math.pi / 180) * length
    """
    cos(x) : -1 ~ +1
    cos(x) * length : -320 ~ +320
    radius + cos(x) * length : 0 ~ +640
    -> 막대기의 x축 범위가 0~640
    """
    y = radius - math.sin(angle * math.pi / 180) * length
    canvas.create_line(x, y, radius, radius, fill='green', width=4)

    # 물체 그리기
    for obj in objects:
        drawObject(obj[0], obj[1])
    """
    drawObject(0, 1) -> (10, 0) -> (20, 0) ... 리스트 끝날 때까지 반복
    """

    # 각도 업데이트
    if direction == 0:
        angle += 1
        if angle == 181:
            direction = 1
    else:
        angle -= 1
        if angle == -1:
            direction = 0

    # 재귀 호출
    canvas.after(10, updateScan)    # 10ms에 한번씩 update

# 화면 표시
updateScan()
root.mainloop()
