Feedback latencies in QuSurf architecture
-----------------------------------------

.. list-table:: Latencies
    :widths: 20 15 25 40
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
        -   synchronize incoming signal on DIO interface to 50 MHz grid. Depends on arrival time and DIO timing calibration
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
        -   output a gate conditional on DSM data
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
        -   perform a break conditional on DSM data
    *   -
        -
        -
        -
    *   -
        -
        -
        -
    *   -   tHdawgTriggerDio
        -   180
        -   HDAWG8 v2, filter disabled, no output delay
        -   delay from DIO trigger to first analog output
    *   -   tHdawgFilter
        -   30
        -
        -   extra delay if the filter is enabled at all (not bypassed)
    *   -   tHdawgFilterHighPass
        -   40
        -
        -   extra delay if high pass filter is enabled
    *   -   tHdawgFilterExpComp
        -   36.67
        -
        -   extra delay per enabled exponential compensation filter stage (8 stages available)
    *   -   tHdawgFilterBounceComp
        -   13.33
        -
        -   extra delay if bounce compensation filter is enabled
    *   -   tHdawgFilterFir
        -   56.67
        -
        -   extra delay if FIR filter is enabled
    *   -   tHdawgOutputDelay
        -   0-TBD
        -
        -   output delay configurable by user (/DEV..../SIGOUTS/n/DELAY)
    *   -
        -
        -
        -
    *   -   tUhfqaTriggerDio
        -
        -
        -   delay from DIO trigger to first analog output. Depends on number of codeword possibilities in sequencing program
    *   -   tUhfqa5stateDiscr
        -   168
        -   UHFQA-5, no bypass
        -   state discrimination latency, from TBD to TBD
    *   -   tUhfqa9stateDiscr
        -   261
        -   UHFQA-9, no bypass
        -   state discrimination latency, from TBD to TBD
    *   -   tUhfqaReadoutProcessing
        -   140
        -   Deskew, Rotation, and Crosstalk units bypassed
        -   delay between the end of a readout pulse at the Signal Inputs and the QA Result Trigger on any Trigger output
    *   -   tUhfqaDeskew
        -   ??
        -
        -   delay introduced by enabling Deskew unit
    *   -   tUhfqaRotation
        -   ??
        -
        -   delay introduced by enabling Rotation unit
    *   -   tUhfqaCrosstalk
        -   ??
        -
        -   delay introduced by enabling Crosstalk unit
    *   -   tUhfqaReadoutProcessing
        -   400
        -   Deskew, Rotation, and Crosstalk units enabled
        -   delay between the end of a readout pulse at the Signal Inputs and the QA Result Trigger on any Trigger output
    *   -   tUhfqaHoldoff
        -
        -
        -
    *   -
        -
        -
        -
    *   -   tQwgTriggerDio
        -   80
        -   using LVDS input
        -   delay from DIO trigger to first analog output. Includes sideband modulation and mixer correction
    *   -
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

FIXME:

- how does tUhfqa*stateDiscr relate to tUhfqaReadoutProcessing?

Information sources:

-   tHdawgTriggerDio: table 5.5 of https://docs.zhinst.com/pdf/ziHDAWG_UserManual.pdf (revision 21.02.0)
-   tHdawgFilter*: section 4.6.2 of same document
-   tCc*: CC-SiteVisitVirtual-20200506.pptx
-   tUhfqaReadoutProcessing: ziUHFQA_UserManual.pdf (revision 21.02.01)
-   tUhf*: QuSurf_MetricsTables_201015-Please-update-for-TEM5.docx
-   tQwg*: 20171511_pitch_qwg_final.pptx

