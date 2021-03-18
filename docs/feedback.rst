Feedback latencies in QuSurf architecture
-----------------------------------------

.. based on:
    QuSurf_MetricsTables_201015-Please-update-for-TEM5.docx
    CC-SiteVisitVirtual-20200506.pptx

.. list-table:: Latencies
    :widths: 25 15 20 40
    :header-rows: 1

    *   -   Identifier
        -   Latency [ns]
        -   Condition
        -   Description
    *   -   tCcInputDio
        -   ~23
        -
        -   delay of DIO input interface and serializer
    *   -   tCcSyncDio
        -   ~10 (0-20)
        -
        -   synchronize incoming signal on DIO interface to 50 MHz grid.
    *   -   tCcDistDsm
        -   20
        -
        -   read DIO interface and dispatch DSM data distribution
    *   -   tCcWaitDsm
        -   80
        -   S-17 (3 parallel 8 bit transfers)
        -   wait for DSM transfers to be completed
    *   -   tCcSyncDsm
        -
        -
        -
    *   -   tCcCondgate
        -   20
        -
        -   output a conditional gate
    *   -   tCcOutputDio
        -   ~10
        -
        -   delay of serializer and DIO output interface
    *   -   **tCcDioToDio**
        -   **~163**
        -   S-17
        -   total latency from DIO data arriving to DIO output, depends on DIO timing calibration
    *   -
        -
        -
        -
    *   -   tCcCondBreak
        -   150
        -
        -   perform a conditional break
    *   -
        -
        -
        -
    *   -
        -
        -
        -
    *   -   tHdawgTriggerDio
        -   150
        -   HDAWG8 v2, filter disabled?
        -   delay from DIO trigger to first analog output
    *   -
        -
        -
        -
    *   -   tUhfqaTriggerDio
        -
        -
        -   delay from DIO trigger to first analog output
    *   -   tUhfqa5stateDiscr
        -   168
        -   UHFQA-5, no bypass
        -   state discrimination latency, from TBD to TBD
    *   -   tUhfqa9stateDiscr
        -   261
        -   UHFQA-9, no bypass
        -   state discrimination latency, from TBD to TBD
    *   -   tUHFQAholdoff
        -
        -
        -
    *   -
        -
        -
        -
    *   -   tVsmDelay
        -   12
        -   VSM v3
        -   delay from digital input to signal starts turning on/off
    *   -   tVsmTransition
        -
        -
        -   transition time of VSM switch from on to off or vice versa
    *   -
        -
        -
        -
