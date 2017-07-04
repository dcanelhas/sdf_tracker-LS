
JAVAPLACE = /home/rts/Pickling/PicklingTools153Release/Java

all :
	javac -cp "${JAVAPLACE}" Pair.java PickleException.java PythonException.java InvalidOpcodeException.java Opcodes.java PrettyPrint.java IObjectConstructor.java PickleUtils.java objects/Time.java objects/TimeDelta.java objects/SetConstructor.java objects/AnyClassConstructor.java objects/ArrayConstructor.java objects/ByteArrayConstructor.java objects/ComplexNumber.java objects/DateTimeConstructor.java Pickler.java IObjectPickler.java UnpickleStack.java Unpickler.java


clean :
	(cd objects && make clean)
	/bin/rm -f *.o *.class 

