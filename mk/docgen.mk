# document related variables
DOC_PATH := docs/html

doc:
	$(VECHO) "  Generate document under the $(DOC_PATH) directory\n"
	$(Q)doxygen Doxyfile
