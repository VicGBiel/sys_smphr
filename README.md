# Painel de Controle de Acesso com FreeRTOS no RP2040

Este projeto implementa um sistema interativo de controle de acesso para gerenciar o número de usuários em um espaço limitado. Foi desenvolvido para a placa RP2040 (como a BitDogLab) utilizando o sistema operacional de tempo real FreeRTOS.

## Funcionalidades Principais

* **Controle de Entrada:** Permite registrar a entrada de usuários através do Botão A.
* **Controle de Saída:** Permite registrar a saída de usuários através do Botão B.
* **Reset do Sistema:** Zera a contagem de usuários através do Botão Z do Joystick.
* **Display OLED:** Exibe o título do painel, o número atual de usuários e mensagens de status (ex: capacidade máxima).
* **LED RGB:** Indica visualmente o nível de ocupação do local:
    * **Azul:** Nenhum usuário.
    * **Verde:** Usuários ativos (com vagas disponíveis).
    * **Amarelo:** Apenas 1 vaga restante.
    * **Vermelho:** Capacidade máxima atingida.
* **Alertas Sonoros (Buzzer):**
    * Beep curto: Tentativa de entrada com o sistema cheio.
    * Beep duplo: Sistema resetado.

## Como Funciona

O sistema utiliza FreeRTOS para gerenciar três tarefas principais: uma para entrada de usuários, uma para saída e uma para o reset do sistema. A sincronização e o gerenciamento de recursos são feitos com:

* **Semáforo de Contagem (`xSlotSemaphore`):** Controla e rastreia o número de usuários ativos.
* **Semáforo Binário (`xResetSemaphore`):** Usado pela rotina de interrupção do botão de reset para sinalizar a tarefa de reset.
* **Mutex (`xDisplayMutex`):** Garante acesso seguro e exclusivo ao display OLED, prevenindo corrupção de dados.

## Video demonstrativo

*[httpshttps://youtu.be/VvR4pXWxvpU]*
