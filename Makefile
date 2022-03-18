# Variables du Makefile
CC = gcc
# Option de compilation
CFLAGS = -std=c11 -Wpedantic -Wall -Wextra -Wconversion -Wwrite-strings -Werror -fstack-protector-all -fpie -D_FORTIFY_SOURCE=2 -O2 -D_XOPEN_SOURCE=700 -D_REENTRANT -g
# Option d'édition de lien
LDFLAGS =
LDLIBS = -Wl,-z,relro,-z,now -pie -lrt -pthread
# Dossiers contenant les sources, objets et éxécutables
SRCDIR = src
OBJDIR = obj
BINDIR = bin
LIBDIR = lib
INCDIR = inc
TESTDIR = test
# Nom des fichiers sans l'extension c et o
SERVER = server
CLIENT = client
ADRESSE_INTERNET = adresse_internet
SOCKET_TCP = socket_tcp
UTILS = utils
TEST_ADRESSE_INTERNET = test_$(ADRESSE_INTERNET)
#--------------------------------------------------------------#
# Construction des dossiers bin, obj, lib et test : make build #
#--------------------------------------------------------------#
build : build_dir
	$(CC) -c ./$(SRCDIR)/$(ADRESSE_INTERNET).c $(CFLAGS) -I $(INCDIR) -o ./$(OBJDIR)/$(ADRESSE_INTERNET).o
	$(CC) -c ./$(SRCDIR)/$(SOCKET_TCP).c $(CFLAGS) -I $(INCDIR) -o ./$(OBJDIR)/$(SOCKET_TCP).o
	$(CC) -c ./$(SRCDIR)/$(UTILS).c $(CFLAGS) -I $(INCDIR) -o ./$(OBJDIR)/$(UTILS).o
build_dir :
	mkdir -p ./$(OBJDIR) ./$(BINDIR) ./$(LIBDIR) ./$(TESTDIR)

#------------------------------------#
# Lancement du serveur : make server #
#------------------------------------#
# Éxécution du fichier éxécutable server
server : ./$(BINDIR)/$(SERVER)
	./$(BINDIR)/$(SERVER)
# Génération de l'éxécutable demon et édition des liens
./$(BINDIR)/$(SERVER) : ./$(OBJDIR)/$(SERVER).o
	$(CC) -o ./$(BINDIR)/$(SERVER) ./$(OBJDIR)/$(SERVER).o ./$(OBJDIR)/$(ADRESSE_INTERNET).o ./$(OBJDIR)/$(SOCKET_TCP).o ./$(OBJDIR)/$(UTILS).o $(LDLIBS)
# Compilation server.c pour générer le fichier objet dans le dossier ./$(OBJDIR)
./$(OBJDIR)/$(SERVER).o : ./$(SRCDIR)/$(SERVER).c
	$(CC) -c ./$(SRCDIR)/$(SERVER).c $(CFLAGS) -I $(INCDIR) -o ./$(OBJDIR)/$(SERVER).o
	
#----------------------------------------------------#
# Test adresse_internet : make test_adresse_internet #
#----------------------------------------------------#
test_adresse_internet : ./$(BINDIR)/$(TEST_ADRESSE_INTERNET)
	./$(BINDIR)/$(TEST_ADRESSE_INTERNET)
./$(BINDIR)/$(TEST_ADRESSE_INTERNET) : ./$(OBJDIR)/$(TEST_ADRESSE_INTERNET).o
	$(CC) -o ./$(BINDIR)/$(TEST_ADRESSE_INTERNET) ./$(OBJDIR)/$(TEST_ADRESSE_INTERNET).o ./$(OBJDIR)/$(ADRESSE_INTERNET).o $(LDLIBS)
./$(OBJDIR)/$(TEST_ADRESSE_INTERNET).o : ./$(TESTDIR)/$(TEST_ADRESSE_INTERNET).c
	$(CC) -c ./$(TESTDIR)/$(TEST_ADRESSE_INTERNET).c $(CFLAGS) -I $(INCDIR) -o ./$(OBJDIR)/$(TEST_ADRESSE_INTERNET).o
	
#-----------#
# Nettoyage #
#-----------#

# clean : supprime les seulement les fichiers objets
clean :
	rm -rf ./$(OBJDIR)/*.o ./$(BINDIR)/* ./$(LIBDIR)/*

# mrproper : supprime le fichier éxécutable
mrproper : clean
	rm -rf ./$(BINDIR)/$(SERVER) ./$(BINDIR)/$(CLIENT) ./$(OBJDIR) ./$(BINDIR) ./$(LIBDIR)
