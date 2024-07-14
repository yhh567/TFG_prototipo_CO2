## Descripción del proyecto

El proyecto consiste en desarrollar un prototipo para la medición de calidad de aire interior. Los parámetros usados en el proyecto para determinar la calidad han sido el dióxido de carbono, 
temperatura y humedad relativa. Para ello se han utilizado dos sensores, MH-Z19B sensor de dióxido de carbono infrarrojo no dispersivo y el sensor BME280 de temperatura y humedad relativa.

El SoC usado en el proyecto fue el ESP8266, ideal para aplicaciones de IoT debido a sus
características:
- conectividad a internet
- bajo costo
- bajo consumo de energia
- amplia variedad de GPIO

El proyecto se puede dividir en tres fases:

- Fase I: en esta parte del desarrollo se comprueba el funcionamiento de los sensores, se desarrolla
el código en el entorno de Arduino. Se opta usar el protocolo MQTT para la transmisión de mensajes
haciendo uso de la arquitectura publicación-suscripción. Se elige Node-RED para crear una interfaz
gráfica sencilla que muestre las mediciones de los sensores en dispositivos móviles o computadoras.

Se eligió una Raspberry Pi para alojar el broker MQTT y Node-RED debido a sus características:
- conectividad a internet
- procesador potente
- RAM suficiente para ejecutar las aplicaciones

- Fase II: la segunda parte del proyecto consiste en el diseño de una pcb que integre ambos sensores
y el microcontrolador. En el diseño de la pcb se ha tenido en cuenta la colocación de los componentes,
el enrutamiento de las pistas para una comunicación estable entre el microcontrolador y los sensores.
Una vez diseñada la pcb se procede a la fabricación del prototipo por parte del técnico de laboratorio
de la Universidad y finalmente se realiza la soldadura de los componentes.

- Fase III: en la última etapa del proyecto se llevó a cabo una serie de pruebas para verificar y validar
el comportamiento esperado del prototipo. Estas pruebas fueron fundamentales para asegurar el
correcto funcionamiento del proyecto y para identificar posibles mejoras.
