# File System FAT12
Este é um projeto de sistema de arquivos FAT12 implementado em C. O objetivo é fornecer uma compreensão básica de como funciona o sistema de arquivos FAT12, incluindo a leitura e escrita de arquivos, manipulação de diretórios e outros.

## Pré-requisitos
* [make](https://www.gnu.org/software/make/)
* [gcc](https://gcc.gnu.org/)


## Compilação

Para compilar o projeto, execute o seguinte comando no terminal:

```bash
make
```

Para compilar e executar utilize:
```bash
make run
```

## Bibliotecas de terceiros

* [stb](https://github.com/nothings/stb): Esse repositório faz uso do `stb_ds.h` para manipulação de arrays dinâmicos.



## Informações extras

### Má formatação do diretório `/SUBDIR/`
Esse diretório tem o first cluster na posição `6` e essa posição na tabela fat contem `0x000` o que significa **Livre** ao invés de `0xFFF` para simbolizar `EOF`

> Por causa desse erro a remoção de arquivos possui um workaround

### _Edge case_ em cópia de arquivos para diretório cheio

Se um arquivo for copiado para um diretório que já está cheio o sistema não fara a alocação de um novo bloco, essa ação resultará em um comportamento indefinido.