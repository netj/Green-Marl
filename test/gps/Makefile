
# stop after parse and typecheck
ODIR = output
COMP = ../../bin/gm_comp
FLAGS = -o=$(ODIR) -t=gps

SRCS = gps1.gm gps2.gm gps3.gm gps4.gm
OUTS =$(addprefix $(ODIR)/, $(SRCS:.gm=.java))
all: $(OUTS)

$(ODIR)/%.java: %.gm
	$(COMP) $(FLAGS) $< 

clean: 
	\rm -rf $(ODIR)/*.java *.java
