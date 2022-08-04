

(emit '
(let ((readFile   "libmscore/breath/breath.mscx")
      (writeFile1 "breath01-test.mscx")
      (reference1 "libmscore/breath/breath01-ref.mscx")
      (writeFile2 "breath02-test.mscx")
      (reference2 "libmscore/breath/breath02-ref.mscx"))

  (let ((score (ms-mtest-readScore readFile))) ; MasterScore* score = readScore(readFile);
    (ms-score-doLayout score) ; score->doLayout();

    (ms-score-startCmd score)  ; score->startCmd();
    (ms-score-cmdSelectAll score) ; score->cmdSelectAll();
    (let ((elms (ms-score-selection-elements score)))
      (do ((e (ms-elements-getnext elms) (ms-elements-getnext elms)))
          ((not e))
        (let ((b (ms-make-breath score))) ; Breath* b = new Breath(score);
          (set! (ms-breath-symId b) 'SymId-breathMarkComma) ; b->setSymId(SymId::breathMarkComma);
          (let ((ed (ms-make-editdata)))
            (ms-editdata-dropelement ed b) ; dd.dropElement = b;
            (ms-element-drop e ed))))) ; if (e->acceptDrop(dd)) e->drop(dd);
    (ms-score-endCmd score) ;  score->endCmd();
    ; QVERIFY(saveCompareScore(score, writeFile1, reference1));
    (ms-test-check
     (ms-mtest-saveCompareScore score writeFile1 reference1))

    ; undo
    (ms-score-undoStack-undo score 0) ; score->undoStack()->undo(0);
    ; QVERIFY(saveCompareScore(score, writeFile2, reference2));
    (ms-test-check
     (ms-mtest-saveCompareScore score writeFile2 reference2))
    (format #t "--- tst_breath DONE ---~%")
    ))
)
