CXX=arm-none-linux-gnueabi-g++

main: main.o edit.o observe.o auth.o curl_common.o buzzer.o dipsw.o fnd.o mled.o bled.o oled.o cled.o tlcd.o receiveSensor.o thread_manager.o display.o touch.o gui.o vision.o
	
	$(CXX) -o main $^ -lcurl -lpthread -I/CNDI_CD/source/application/opencvApp/build/install/include -I/CNDI_CD/source/application/opencvApp/build/install/include/opencv2 -L/CNDI_CD/source/application/opencvApp/build/install/lib -lopencv_core -lopencv_flann -lopencv_highgui -lopencv_imgproc

main.o: main.cpp
	$(CXX) -c main.cpp

edit.o: edit.cpp
	$(CXX) -c edit.cpp

observe.o: observe.cpp
	$(CXX) -c observe.cpp

auth.o: auth.cpp
	$(CXX) -c auth.cpp

curl_common.o: curl_common.cpp
	$(CXX) -c curl_common.cpp

buzzer.o: buzzer.cpp
	$(CXX) -c buzzer.cpp

dipsw.o: dipsw.cpp
	$(CXX) -c dipsw.cpp

fnd.o: fnd.cpp
	$(CXX) -c fnd.cpp

mled.o: mled.cpp
	$(CXX) -c mled.cpp

bled.o: bled.cpp
	$(CXX) -c bled.cpp

oled.o: oled.cpp
	$(CXX) -c oled.cpp

cled.o: cled.cpp
	$(CXX) -c cled.cpp

tlcd.o: tlcd.cpp
	$(CXX) -c tlcd.cpp

receiveSensor.o: receiveSensor.cpp
	$(CXX) -c receiveSensor.cpp

touch.o: gui/touch.cpp
	$(CXX) -c gui/touch.cpp

thread_manager.o: thread_manager.cpp
	$(CXX) -c thread_manager.cpp


display.o: gui/display.cpp
	$(CXX) -c gui/display.cpp

gui.o: gui/gui.cpp
	$(CXX) -c gui/gui.cpp

vision.o: vision/vision.cpp
	$(CXX) -c vision/vision.cpp -I/CNDI_CD/source/application/opencvApp/build/install/include -I/CNDI_CD/source/application/opencvApp/build/install/include/opencv2 -L/CNDI_CD/source/application/opencvApp/build/install/lib -lopencv_core -lopencv_flann -lopencv_highgui -lopencv_imgproc

clean:
	rm *.o