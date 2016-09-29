### msm8974pro-ac-pm-8974-a0001.dts

- "msm8974pro-ac-pm8941.dtsi"

    - msm8974pro-pm8941.dtsi
    
        - "msm8974pro.dtsi"
        
            - "msm8974.dtsi"
            
                - "skeleton.dtsi"
                - "msm8974-camera.dtsi"
                - "msm8974-coresight.dtsi"
                - "msm-gdsc.dtsi"
                - "msm8974-ion.dtsi"
                - "msm8974-gpu.dtsi"
                - "msm8974-mdss.dtsi"
                
                    - "msm8974-mdss-panels.dtsi"
                    
                        - "dsi-panel-orise-720p-video.dtsi"
                        - "dsi-panel-toshiba-720p-video.dtsi"
                        - "dsi-panel-sharp-qhd-video.dtsi"
                        - "dsi-panel-generic-720p-cmd.dtsi"
                        - "dsi-panel-jdi-1080p-video.dtsi"
                        - "dsi-panel-jdi-dualmipi0-video.dtsi"
                        - "dsi-panel-jdi-dualmipi1-video.dtsi"
                        - "dsi-panel-jdi-dualmipi0-cmd.dtsi"
                        - "dsi-panel-jdi-dualmipi1-cmd.dtsi"
                    
                - "msm8974-smp2p.dtsi"
                - "msm8974-bus.dtsi"
                - "msm-rdbg.dtsi"
                
                - slim_msm: slim@fe12f000

		            - taiko_codec
                
                - sound
                
            - "msm8974-v2-iommu.dtsi"
            
                - "msm-iommu-v1.dtsi"
                
            - "msm8974-v2-iommu-domains.dtsi"
            - "msm8974pro-pm.dtsi"
            - "msm8974pro-ion.dtsi"
            
        - "msm-pm8x41-rpm-regulator.dtsi"
        - "msm-pm8841.dtsi"


        - "msm-pm8941.dtsi"
        - "msm8974-regulator.dtsi"
        - "msm8974-clock.dtsi"
        
- "msm8974-oppo/msm8974-a0001.dtsi"

    - "msm8974-oppo-common.dtsi"
    
        - "msm8974-oppo-panel.dtsi"
        
            - "dsi-panel-jdi-1080p-cmd.dtsi"
            - "dsi-panel-jdi-1080p-video.dtsi"
            - "dsi-panel-sharp-1080p-cmd.dtsi"
            - "dsi-panel-truly-1080p-cmd.dtsi"
            - "dsi-panel-truly-1080p-video.dtsi"

            - "dsi-panel-jdi-1440p-video.dtsi"
            - "dsi-panel-rsp-1440p-cmd.dtsi"
            - "dsi-panel-rsp-1440p-video.dtsi"
        
        - "msm8974-oppo-camera-sensor.dtsi"
        
    - "msm8974-oppo-camera.dtsi"
    - "msm8974-a0001-camera-sensor.dtsi"
    - "msm8974-oppo-input.dtsi"
    - "msm8974-oppo-usb.dtsi"
    - "msm8974-oppo-misc.dtsi"
    - "msm8974-oppo-pm.dtsi"
    - "msm8974-oppo-nfc.dtsi"
    - "msm8974-oppo-regulator.dtsi"
    - "msm8974-oppo-sound.dtsi"
    
        - slim@fe12f000
        
            - taiko_codec
            
        - sound
        
    - "msm8974-a0001-sound.dtsi"
    
        - sound
        
        - &slim_msm
        
            - taiko_codec
        
    - "msm8974-oppo-leds.dtsi"
    
        - "../msm8974-leds.dtsi"
        

### msm8974-v1-cdp.dts
        
- "msm8974-v1.dtsi"
    - "msm8974.dtsi"

    - "msm-pm8x41-rpm-regulator.dtsi"
    - "msm-pm8841.dtsi"
    
    - "msm-pm8941.dtsi"
    - "msm8974-regulator.dtsi"
    - "msm8974-clock.dtsi"

    - "msm8974-v1-iommu.dtsi"
        - "msm-iommu-v1.dtsi"
        
    - "msm8974-v1-iommu-domains.dtsi"
    - "msm8974-v1-pm.dtsi"
    
- "msm8974-cdp.dtsi"
    - "msm8974-leds.dtsi"
    - "msm8974-camera-sensor-cdp.dtsi"
    
### msm8974-v1-fluid.dts

- "msm8974-v1.dtsi"

- "msm8974-fluid.dtsi"

    - "msm8974-camera-sensor-fluid.dtsi"
    - "msm8974-leds.dtsi"
    
### msm8974-v1-liquid.dts

- "msm8974-v1.dtsi"
- "msm8974-liquid.dtsi"

    - "msm8974-leds.dtsi"
    - "msm8974-camera-sensor-liquid.dtsi"
    
### msm8974-v1-mtp.dts
- "msm8974-v1.dtsi"

- "msm8974-mtp.dtsi"

    - "msm8974-camera-sensor-mtp.dtsi"
    - "msm8974-leds.dtsi"
    
    mtp_batterydata: qcom,battery-data {
        - "batterydata-palladium.dtsi"
        - "batterydata-mtp-3000mah.dtsi"
    };

### msm8974-v1-rumi.dts

- "msm8974-v1.dtsi"
- "msm8974-rumi.dtsi"

    - "msm8974-leds.dtsi"
    - "msm8974-camera-sensor-cdp.dtsi"
    
### msm8974-v1-sim.dts

- "msm8974-v1.dtsi"
- "msm8974-sim.dtsi"

    - "dsi-panel-sim-video.dtsi"
    - "msm8974-leds.dtsi"
    - "msm8974-camera-sensor-cdp.dtsi"
    
### msm8974-v2.0-1-cdp.dts

- "msm8974-v2.0-1.dtsi"

    - "msm8974-v2.dtsi"
    
        - "msm8974.dtsi"

        - "msm-pm8x41-rpm-regulator.dtsi"
        - "msm-pm8841.dtsi"

        - "msm-pm8941.dtsi"
        - "msm8974-regulator.dtsi"
        - "msm8974-clock.dtsi"

        - "msm8974-v2-iommu.dtsi"
        - "msm8974-v2-iommu-domains.dtsi"
        - "msm8974-v2-pm.dtsi"
        
- "msm8974-cdp.dtsi"

    - "msm8974-leds.dtsi"
    - "msm8974-camera-sensor-cdp.dtsi"

### msm8974-v2.0-1-fluid.dts

- "msm8974-v2.0-1.dtsi"

    - "msm8974-v2.dtsi"
    
        - "msm8974.dtsi"

        - "msm-pm8x41-rpm-regulator.dtsi"
        - "msm-pm8841.dtsi"

        - "msm-pm8941.dtsi"
        - "msm8974-regulator.dtsi"
        - "msm8974-clock.dtsi"

        - "msm8974-v2-iommu.dtsi"
        - "msm8974-v2-iommu-domains.dtsi"
        - "msm8974-v2-pm.dtsi"

- "msm8974-fluid.dtsi"

    - "msm8974-camera-sensor-fluid.dtsi"
    - "msm8974-leds.dtsi"
    
### msm8974-v2.0-1-liquid.dts
    
- "msm8974-v2.0-1.dtsi"

    - "msm8974-v2.dtsi"
    
        - "msm8974.dtsi"

        - "msm-pm8x41-rpm-regulator.dtsi"
        - "msm-pm8841.dtsi"

        - "msm-pm8941.dtsi"
        - "msm8974-regulator.dtsi"
        - "msm8974-clock.dtsi"

        - "msm8974-v2-iommu.dtsi"
        - "msm8974-v2-iommu-domains.dtsi"
        - "msm8974-v2-pm.dtsi"

- "msm8974-liquid.dtsi"

    - "msm8974-leds.dtsi"
    - "msm8974-camera-sensor-liquid.dtsi"
    

### msm8974-v2.0-1-mtp.dts

- "msm8974-v2.0-1.dtsi"

    - "msm8974-v2.dtsi"
    
        - "msm8974.dtsi"

        - "msm-pm8x41-rpm-regulator.dtsi"
        - "msm-pm8841.dtsi"

        - "msm-pm8941.dtsi"
        - "msm8974-regulator.dtsi"
        - "msm8974-clock.dtsi"

        - "msm8974-v2-iommu.dtsi"
        - "msm8974-v2-iommu-domains.dtsi"
        - "msm8974-v2-pm.dtsi"

- "msm8974-mtp.dtsi"

    - "msm8974-camera-sensor-mtp.dtsi"
    - "msm8974-leds.dtsi"
    
    mtp_batterydata: qcom,battery-data {
        - "batterydata-palladium.dtsi"
        - "batterydata-mtp-3000mah.dtsi"
    };
    
### apq8074-v2.0-1-cdp.dts
    
- "apq8074-v2.0-1.dtsi"

    - "msm8974-v2.0-1.dtsi"

        - "msm8974-v2.dtsi"
        
            - "msm8974.dtsi"

            - "msm-pm8x41-rpm-regulator.dtsi"
            - "msm-pm8841.dtsi"

            - "msm-pm8941.dtsi"
            - "msm8974-regulator.dtsi"
            - "msm8974-clock.dtsi"

            - "msm8974-v2-iommu.dtsi"
            - "msm8974-v2-iommu-domains.dtsi"
            - "msm8974-v2-pm.dtsi"
    
    - "apq8074-v2.0-1-ion.dtsi"

- "msm8974-cdp.dtsi"

    - "msm8974-leds.dtsi"
    - "msm8974-camera-sensor-cdp.dtsi"


### apq8074-v2.0-1-liquid.dts

- "apq8074-v2.0-1.dtsi"

    - "msm8974-v2.0-1.dtsi"

        - "msm8974-v2.dtsi"
        
            - "msm8974.dtsi"

            - "msm-pm8x41-rpm-regulator.dtsi"
            - "msm-pm8841.dtsi"

            - "msm-pm8941.dtsi"
            - "msm8974-regulator.dtsi"
            - "msm8974-clock.dtsi"

            - "msm8974-v2-iommu.dtsi"
            - "msm8974-v2-iommu-domains.dtsi"
            - "msm8974-v2-pm.dtsi"
    
    - "apq8074-v2.0-1-ion.dtsi"
    
- "msm8974-liquid.dtsi"

    - "msm8974-leds.dtsi"
    - "msm8974-camera-sensor-liquid.dtsi"
    
### apq8074-v2.0-1-dragonboard.dts    

- "apq8074-v2.0-1.dtsi"

    - "msm8974-v2.0-1.dtsi"

        - "msm8974-v2.dtsi"
        
            - "msm8974.dtsi"

            - "msm-pm8x41-rpm-regulator.dtsi"
            - "msm-pm8841.dtsi"

            - "msm-pm8941.dtsi"
            - "msm8974-regulator.dtsi"
            - "msm8974-clock.dtsi"

            - "msm8974-v2-iommu.dtsi"
            - "msm8974-v2-iommu-domains.dtsi"
            - "msm8974-v2-pm.dtsi"
    
    - "apq8074-v2.0-1-ion.dtsi"
    
- "apq8074-dragonboard.dtsi"

    - "dsi-panel-sharp-qhd-video.dtsi"
    - "msm8974-camera-sensor-dragonboard.dtsi"
    - "msm8974-leds.dtsi"
    - "msm-rdbg.dtsi"
    
### apq8074-v2.2-cdp.dts   
 
- "apq8074-v2.2.dtsi"

    - "msm8974-v2.2.dtsi"
    
        - "msm8974-v2.dtsi"
        
            - "msm8974.dtsi"

            - "msm-pm8x41-rpm-regulator.dtsi"
            - "msm-pm8841.dtsi"

            - "msm-pm8941.dtsi"
            - "msm8974-regulator.dtsi"
            - "msm8974-clock.dtsi"

            - "msm8974-v2-iommu.dtsi"
            - "msm8974-v2-iommu-domains.dtsi"
            - "msm8974-v2-pm.dtsi"
        
    - "apq8074-v2.2-ion.dtsi"

- "msm8974-cdp.dtsi"

    - "msm8974-leds.dtsi"
    - "msm8974-camera-sensor-cdp.dtsi"
    
### apq8074-v2.2-dragonboard.dts

- "apq8074-v2.2.dtsi"

    - "msm8974-v2.2.dtsi"
    
        - "msm8974-v2.dtsi"
        
            - "msm8974.dtsi"

            - "msm-pm8x41-rpm-regulator.dtsi"
            - "msm-pm8841.dtsi"

            - "msm-pm8941.dtsi"
            - "msm8974-regulator.dtsi"
            - "msm8974-clock.dtsi"

            - "msm8974-v2-iommu.dtsi"
            - "msm8974-v2-iommu-domains.dtsi"
            - "msm8974-v2-pm.dtsi"
        
    - "apq8074-v2.2-ion.dtsi"
    
- "apq8074-dragonboard.dtsi"

    - "dsi-panel-sharp-qhd-video.dtsi"
    - "msm8974-camera-sensor-dragonboard.dtsi"
    - "msm8974-leds.dtsi"
    - "msm-rdbg.dtsi"
    
### msm8974-v2.2-cdp.dts
  
- "msm8974-v2.2.dtsi"
- "msm8974-cdp.dtsi"

### msm8974-v2.2-fluid.dts
- "msm8974-v2.2.dtsi"
- "msm8974-fluid.dtsi"

### msm8974-v2.2-liquid.dts

- "msm8974-v2.2.dtsi"
- "msm8974-liquid.dtsi"

### msm8974-v2.2-mtp.dts

- "msm8974-v2.2.dtsi"
- "msm8974-mtp.dtsi"

### msm8974pro-ab-pm8941-cdp.dts

- "msm8974pro-ab-pm8941.dtsi"
    - "msm8974pro-pm8941.dtsi"
- "msm8974-cdp.dtsi"

### msm8974pro-ab-pm8941-fluid.dts

- "msm8974pro-ab-pm8941.dtsi"
    - "msm8974pro-pm8941.dtsi"
- "msm8974-fluid.dtsi"

### msm8974pro-ab-pm8941-mtp.dts

- "msm8974pro-ab-pm8941.dtsi"
    - "msm8974pro-pm8941.dtsi"
- "msm8974-mtp.dtsi"

### msm8974pro-ab-pm8941-fluid-hdpt.dts

- "msm8974pro-ab-pm8941.dtsi"
    - "msm8974pro-pm8941.dtsi"
- "msm8974-fluid.dtsi"
- "dsi-panel-jdi-720p-cmd.dtsi"

### msm8974pro-ab-pm8941-liquid.dts

- "msm8974pro-ab-pm8941.dtsi"
    - "msm8974pro-pm8941.dtsi"
- "msm8974-liquid.dtsi"

### msm8974pro-ac-pm8941-cdp.dts

- "msm8974pro-ac-pm8941.dtsi"
- "msm8974-cdp.dtsi"

### msm8974pro-ac-pm8941-liquid.dts

- "msm8974pro-ac-pm8941.dtsi"
- "msm8974-liquid.dtsi"

### msm8974pro-ac-pm8941-liquid.dts

- "msm8974pro-ac-pm8941.dtsi"
- "msm8974-liquid.dtsi"

### msm8974pro-ac-pm8941-mtp.dts

- "msm8974pro-ac-pm8941.dtsi"
- "msm8974-mtp.dtsi"

### msm8974pro-ac-pma8084-pm8941-mtp.dts

- "msm8974pro-ac-pma8084-pm8941.dtsi"
    - "msm8974pro-ac-pma8084.dtsi"
        - "msm8974pro-pma8084.dtsi"
            - "msm8974pro.dtsi"

            - "msm-pma8084-rpm-regulator.dtsi"
            - "msm-pma8084.dtsi"
            - "msm8974pro-pma8084-regulator.dtsi"

    
    - "msm-pm8941.dtsi"
    
- "msm8974pro-pma8084-mtp.dtsi"
    - "msm8974-mtp.dtsi"