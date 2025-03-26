                                    Bare - metal programming

Dans ce document, on peut retrouver toutes mes connaissances autour du bare-mental programming : 
Les différentes mémoires et le fonctionnement de la partie Hardware. 

- Comment fonctionnent QEMU et les différents fichiers sources ?

QEMU est un emulateur qui simule la machine (CPU, mémoires et périphériques) et peut executer du code binaire pour une architecture cible. 
Quand je lance ma machine simulé cette dernière ne contient rien. Elle ne sait ni quoi executer ni où. C'est pourquoi quand on lance QEMU il faut lui associer un fichier .elf contenant les informations sur l'organisation mémoire, le point d'entrée et le code machine.
Contenu et travail du .elf: En lissant le Makefile, je vois que make run va nous permettre de lancer QEMU avec le fichier kernel.elf. Ce fichier contient tous les executables (exception.o, main.o, startup.o, uart.o) qui on étaient regroupé grace au linker kernel.ld qui permet de définir l'organisation mémoire . Ce dernier defini les sections de stockage du code executable (.text), des variables initialisées (.data), des variables non initialisées (.bss) et de la pile (stack). 

- Mais si la machine ne connait rien où est chargé et executé ce .elf ?
Lorsque l'on lance quemu avec notre fichier kernel.elf ce dernier est chargé et exécuté à l'adresse défini par le linker pour nous 0x0. Parmis le contenu de kernel.elf c'est le contenu de startup.s qui est exécuté en premier. 

- Qu'est ce que startup.s ?
startup initialise notre systeme pour qu'il puisse executer notre noyau (main.c). 
Au début de ce dernier, on retrouve la définision des constantes qui définisse les modes possible pour le processeur (utilisateur,system, interuption) et de désactiver les interuptions . La constante qui correspond au mode actuel et celles qui correspondent aux autorisations d'interuptions sont stoquées dans le le registre CPSR. Registre qui permet de connaitre l'état du processeur.
On configure alors le processeur en mode systeme avec les interuptions désactivées. Avant de charger l'adresse du début de la pile dans le registre sp et d'initialisé les variables non initialisées à 0. Puis de sauter à l'adresse de la fonction _start qui ce trouve dans notre main.c.

- que fais _start ?
il commence par vérifier que la pile n'est pas rempli. Puis initialise les uarts avant de faire une boucle qui attend de recevoir quelque chose dans le port uart et de le renvoyer.
Initialisé les uarts revient à enregistrer leurs numéros et adresses de départ dans un tableau.
ensuite on appel uart_enable du port UART0 qui nous permettra des gerer les interruptions mais qui pour l'instant de fait rien. Enfin on rentre dans une boucle qui fait appel à uart_receive et uart_send sur uartO.

uart_receive regarde si le 4 eme bit du registre de flag est à 1 en appliquant un "et" bit à bit entre ce dernier et un masque où seul le quatrième bit est à un 1. Tant que le resultat de l'opération est 1. C'est à dire tant que le 4 eme bit du registre de flag est à 1 alors le fifo de reception est vide et on attend. Lorsque le fifo de reception n'est plus vide on charge le caractère présent à l'adresse donnée en argument depuis l'adresse du fifo. 

uart_send regarde si le 5 eme bit du registre de flag est à 1 si oui cela signifie que le registre fifo est pleins. On attend tant que c'est le cas et écrit un caractaire donnée en argument à l'adresse du fifo.


- Pour pouvoir accéder au port uart et au registre fifo et flags il faut donc connaitre leur adresse et les configurer dans uart-mmio.h
J'ai pu trouver ces adresses dans la documentation. On a donc :

#define UART0_BASE_ADDRESS ((void*)0x101F1000)
#define UART1_BASE_ADDRESS ((void*)0x101F2000) 
#define UART2_BASE_ADDRESS ((void*)0x101F3000) 

// adresse du registre fifo
#define UART_DR 0x00 

//adresse du registre de flag
#define UART_FR 0x18

Chaque uart à une adresse de départ et à partir de celle-ci, l'adresse des registre flag et fifo représente un décalage par rapport à cette adresse. Le registre flag de l'uartO ce trouve donc à l'adresse 0x101F1018.

- Où est ce que on écrit réellement dans quel mémoire ? Quelles sont les adresses où j'écrit ? 

Pour comprendre cela je me suis penché sur le tableau de mappage des adresses que l'on retrouve slide 22 du cours (Lecture1.pdf)

Chaque système à une procédure de mappage des adresses afin d'attribuer différentes plages d'adresses à différentes fonctionnalité de ce dernier. Selon la plage d'adresse utilisé on accède à différentes mémoire physique. Ainsi, les registres de UART sont stockés dans une mémoire reservées aux périphériques , sur ces derniers. Elle est mappé et accéssible graces aux adresses si dessus. 


- Quelles sont les différentes mémoires ? Quels sont leurs roles et qui y accèdent?

Un SoC est une puce qui contient tout ce que un ordinateur aurait besoin pour fonctionner: CPU, mémoire, periphériques, bus de communication et FPGA.
La mémoire du SoC est diviser en deux la RAM et la ROM.
La ram correspond aux mémoire vive et volatile. C'est à dire que cette derniere est vidé lorsque l'on arrete le systeme. Dans la ram on retrouve l'OCM qui est interne au processeur et sert à stoquer des données critiques et d'amorçage comme le firmware ou le bootloader au démarage. Elle est très rapide et ne contient que quelque Ko-Mo. On retrouve aussi parmis la RAM, la DDR est externe est connecté via bus mémoire. Elle permet de stocker le code et les applications en cours d'execution et les données volumineuse. Cette dernière est plus lente que l'OCM mais plus grande (Go)

Ensuite, on retrouve la mémoire ROM ou flash qui est non volatile. Ici, c'est une mémoire Quad-SPI qui est utilisé. Il existe d'autres mémoire ROM comme la mémoire NAND qui est moins cher mais moins rapide. C'est deux mémoires flash sont plus lente que les mémoires RAM mais on de plus grande capacité. Elles permettent de stoquer l'OS, le firmware et les fichiers utilisateurs. C'est dernière sont connectées au serveur grace aux plages d'adresse SMC qui permettent de connecter les mémoires externes au serveur.

Dans le systeme de mappage des adresses on retrouve aussi des plages d'adresses pour la partie PL qui correspond à la partie FPGA d'un SoC. La FPGA est une puce qui permet de reprogrammer du matériel. On retrouve aussi la plage IOP qui correspond aux adresses des périphériques (comme UART), la plage des registre SLCR qui permettent de configurer les composants du SoC. Par exemple de modifier la fréquence du CPU, des bus, d'activer ou de désactiver des composants. Et la plage d'adresse des registres PS qui controle et configure le processeur, les périphériques et la mémoire. On y retrouve les CR pour configurer le processeur, IR pour gérer les intéruptions, les SR qui connaissent l'état du processeur et le MMR qui gère les interactions entre l'espace d'adressage du processeur et l'espace physique de la mémoire.

- Qu'est ce qui ce passe lorsque j'allume mon ordinateur ? Qu'est ce qui ce lance ? Qui est responssable de quel partie logiciel, matériel ?

- Comment fonctionnent les intéruptions ? 






