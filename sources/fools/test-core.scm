;(display "Minimal system ready!\n")

(display "Core tests\n")
(display (eq? (Object 'superclass) null))
(display (eq? (Object_class 'superclass) class))
(display (eq? (class 'superclass) classBehaviour))
(display (eq? ((class 'delegate) 'superclass)
              (classBehaviour 'delegate)))
(display (eq? (Metaclass_class 'superclass) (classBehaviour 'class)))
(display (eq? (Metaclass 'superclass) classBehaviour))
(display (eq? (classBehaviour 'superclass) Object))
(display (eq? ((classBehaviour 'delegate) 'superclass)
              Object_class))
(display (eq? (Object_class 'delegate) Metaclass))
(display (eq? ((Metaclass 'delegate) 'delegate) Metaclass))

(display "Environment tests\n")
(let ((el (Env 'scope:key: 4 Env))
      (el2 (Env 'scope:key: 3 IFixed)))
    (el2 'parent: el)
    (display (eq? (el 'parent) null))
    (display (eq? (el2 'parent) el)))


;(display "STAGE 5\n")

;((Object 'new) 'print)
;(Object 'print)
;(Metaclass 'instance)
;(Object_class 'basicNew) ; Metaclasses don't have a NEW. The "NEW" is
                         ; only generated on the spot to create its
                         ; single instance in "newclass"
;((class 'new) 'print)
 

;((integer 'methodDictionary)
;    'objectAt:put: 'testMethod
;    (method (s) (display "IN SELF!\n")
;                ((getsuper s) (vector 'testMethod))))
;((magnitude 'methodDictionary)
;    'objectAt:put: 'testMethod
;    (method (s) (display "In SUPER!!\n")))

;((integer 'basicNew) 'testMethod)


;(display ((ICapture 'instance) 'eval))

(SmallInteger 'store:method: +
    (method (s other)
        (+ (getself s) other)))

;(let* ((file (InputFile 'on: "fib-compiler.scm"))
;       (size (file 'size))
;       (s (String 'basicNew: size)))
;    (let loop ((current 0))
;        (if (= current size)
;            (display s)
;            (begin
;                (s 'objectAt:put: current (file 'readChar))
;                (loop (+ current 1))))))
(let* ((file (InputFile 'on: "fib-compiler.scm"))
       (s (file 'readAllChars)))
    (display s))

;(let ((s "boe\n"))
;    (s 'objectAt:put: 2 #\ƺ)
;    (display #\ƺ)
;    (display s))

;(InputFile 'dispatch:delegate:
;            objdisp
;            (make_empty_subclass IFile 'UTF8InputFile))
;(OutputFile 'dispatch:delegate:
;            objdisp
;            (make_empty_subclass OFile 'UTF8OutputFile))

;(InputFile 'test)

;(display "blaboe\n")

;(display (IList 'basicNew: 4))

;(display (1 + 2))

;(string 'store:method: 'testMethod
;    (method (s) (display "HELLO!") (display (getself s))
;                (display "\n")))

;("biep" 'testMethod)
;('testMethod 'testMethod)

;(symbol 'store:method: 'testMethod
;    (method (s) (display "2H!") (display (getself s))
;                (display "\n")))

;("biep" 'testMethod)
;('testMethod 'testMethod)

;(let ((test "BOE\n"))
;    ((string 'methodDictionary) 'objectAt:put:
;        'testMethod (method (s)
;            (display "bla\n")
;            (display (getself s))
;            (display (getself s))
;            (display (getself s))
;            (display "biep\n")))
;    (test 'testMethod))

;(let ((test (array 'basicNew: 10)))
;    ((array 'methodDictionary) 'objectAt:put:
;        'testMethod (method (s) (display "BOE\n")
;                                (display ((getself s) 'size))))
;    (display "DO test\n")
;    (test 'testMethod)
;    (display "done test\n")
;    )
        
;(display "SYSTEM READY\n")


