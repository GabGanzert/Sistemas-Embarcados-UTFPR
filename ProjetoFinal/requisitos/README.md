# Sistema de elevadores - Requisitos 
## Funcionais
- Se houver uma solicitação para o elevador descer, apenas um elevador que já estiver descendo, ou que estiver parado, irá atendê-la. O mesmo vale para uma solicitação de subida. 
- Caso uma solicitação para descer ocorra, e o andar escolhido seja um andar superior, esta solicitação será a última a ser atendida (caso não haja nenhuma outra solicitação interna nem externa, ela será atendida imediatamente). O mesmo vale para uma solicitação de subida.
- Se o andar solicitado for o andar presente, nenhuma ação deve ser tomada.
- Um elevador só deve parar quando sua cabine estiver alinhada com um andar.
- Ao chegar ao andar de destino e este for diferente do térreo, o elevador deve abrir a porta e permanecer com esta aberta por sete segundos. 
- Se nenhuma solicitação existir ou for feita dentro desse intervalo de tempo, o elevador deve fechar a porta e retornar ao térreo. Caso seja o térreo, a porta deve permanecer aberta até que uma nova solicitação seja recebida.
- A porta só pode ser aberta ou fechada com o elevador parado.
- Quando um andar é selecionado, a luz do respectivo botão deve ser acesa. Ao chegar ao destino, deve ser apagada.
- Caso dois ou mais elevadores atendam aos requisitos para atender uma solicitação, o elevador do botão apertado será escolhido, ou o mais próximo deste. Em último caso, o da esquerda terá preferência.
 
## Não funcionais
- O microcontrolador usado deve ser o EK-TM4C1294XL.
- O código deve ser desenvolvido em linguagem C.
- O rtos usado deve ser o Keil RTX5.
- Deve haver ao menos uma thread para cada elevador.
- O baud rate da comunicação serial entre microcontrolador e simulador deve ser de 115200 bps.
- A comunicação serial (UART) deve ser implementada por interrupção, tanto na recepção quanto na transmissão de caracteres.
