APP=harris

include ../common.mk

$(APP): $(APP)_opt.so
	rm -f $(APP).so
	ln -s $< $(APP).so
	python2 main.py ../../images/grand_canyon2.jpg 11
	#python2 main.py ../../images/venice_wikimedia.jpg 11

$(APP)_naive: $(APP)_naive.so
	rm -f $(APP).so
	ln -s $< $(APP).so
	python2 main.py ../../images/grand_canyon2.jpg 11
	#python2 main.py ../../images/venice_wikimedia.jpg 11
