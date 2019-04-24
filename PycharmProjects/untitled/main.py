#!/usr/bin/env python

import bluetooth
import pygame


BLACK = (0, 0, 0)
WHITE = (255, 255, 255)


# uebernommen als zeichenhilfe
class TextPrint:
    def __init__(self):
        self.reset()
        self.font = pygame.font.Font(None, 20)

    def ausgabe(self, screen, textString):
        textBitmap = self.font.render(textString, True, BLACK)
        screen.blit(textBitmap, [self.x, self.y])
        self.y += self.line_height

    def reset(self):
        self.x = 10
        self.y = 10
        self.line_height = 15

    def indent(self):
        self.x += 10

    def unindent(self):
        self.x -= 10


# init controller
pygame.init()
# screen config
done = 1
size = [500, 700]
screen = pygame.display.set_mode(size)

# pygame.joystick.init()
controller = pygame.joystick.Joystick(0)
controller.init()
pygame.display.set_caption("My Game")

# Used to manage how fast the screen updates
clock = pygame.time.Clock()

# Initialize the joysticks
pygame.joystick.init()

# Get ready to print
textPrint = TextPrint()

print 'Xbox Controller Connected'
print controller.get_name()

# Create the client socket
MAC_ADR = "3C:71:BF:A6:E3:5E"
client_socket = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
client_socket.connect((MAC_ADR, 1))
print "Bluetooth Connected"
print ' '
print ' '

print '/**************************/'
print 'Joystick Drive Program'
print "Press 'q' to quit"
print '/**************************/'

print controller.get_init()
key = 0
y = 0
x = 0

while done:

    for event in pygame.event.get():  # abfrage ob etwas getan wurde
        if event.type == pygame.QUIT:
            done = 0

        # moeglichkeiten der eingabe: JOYAXISMOTION JOYBALLMOTION JOYBUTTONDOWN JOYBUTTONUP JOYHATMOTION
        if event.type == pygame.JOYBUTTONDOWN:
            client_socket.send("send")   # erstmal nur das runterdruecken als funktion sehen
            print("Joystick button pressed.")
        if event.type == pygame.JOYBUTTONUP:
            print("Joystick button released.")

    # Zeichnen des feldes
    screen.fill(WHITE)
    textPrint.reset()

    # universal einlesen der joysticks
    joystick_count = pygame.joystick.get_count()

    textPrint.ausgabe(screen, "Number of joysticks: {}".format(joystick_count))
    textPrint.indent()

    # fuer alle joysticks initialisieren und zeichnen.:
    for i in range(joystick_count):
        joystick = pygame.joystick.Joystick(i)
        joystick.init()

        textPrint.ausgabe(screen, "Joystick {}".format(i))
        textPrint.indent()

        # danach die daten des Joysticks zuschreiben
        name = joystick.get_name()
        textPrint.ausgabe(screen, "Joystick name: {}".format(name))

        # achsen funktionieren in 2 richtungen. zum zaehlen wieviele vorhanden sind
        axes = joystick.get_numaxes()
        textPrint.ausgabe(screen, "Number of axes: {}".format(axes))
        textPrint.indent()

        for i in range(axes):
            axis = joystick.get_axis(i)
            textPrint.ausgabe(screen, "Axis {} value: {:>6.3f}".format(i, axis))
            # dadurch das achsen allgemein zwischen -1 und 1 laufen in 3 stellen hinter komma format.
        textPrint.unindent()

        # knoepfe zaehlen
        buttons = joystick.get_numbuttons()
        textPrint.ausgabe(screen, "Number of buttons: {}".format(buttons))
        textPrint.indent()
        # knoepfe nacheinander ausgeben
        for i in range(buttons):
            button = joystick.get_button(i)
            textPrint.ausgabe(screen, "Button {:>2} value: {}".format(i, button))
        textPrint.unindent()

        # Hat switch. auch steuerkreutz genannt. zaehlen und auch ausgeben
        hats = joystick.get_numhats()
        textPrint.ausgabe(screen, "Number of hats: {}".format(hats))
        textPrint.indent()

        for i in range(hats):
            hat = joystick.get_hat(i)
            textPrint.ausgabe(screen, "Hat {} value: {}".format(i, str(hat)))
        textPrint.unindent()

        textPrint.unindent()

    # nichtveraenderte daten sollen beibleiben befohr neu gezeichnet wird
    pygame.display.flip()
    clock.tick(20)

    # print command
    # client_socket.send(command)
    # print client_socket.recv(1024)

# verbindung kappen
client_socket.close()
