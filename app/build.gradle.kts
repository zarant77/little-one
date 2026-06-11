plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.catemup.littleone"

    compileSdk {
        version = release(36) {
            minorApiLevel = 1
        }
    }

    defaultConfig {
        applicationId = "com.catemup.littleone"
        minSdk = 24
        targetSdk = 36
        versionCode = 2
        versionName = "1.0"

        ndk {
            abiFilters += "arm64-v8a"
        }
    }

    signingConfigs {
        create("release") {
            val storeFilePath = providers.gradleProperty("LITTLE_ONE_STORE_FILE").orNull
            val storePass = providers.gradleProperty("LITTLE_ONE_STORE_PASSWORD").orNull
            val alias = providers.gradleProperty("LITTLE_ONE_KEY_ALIAS").orNull
            val keyPass = providers.gradleProperty("LITTLE_ONE_KEY_PASSWORD").orNull

            if (
                storeFilePath != null &&
                storePass != null &&
                alias != null &&
                keyPass != null
            ) {
                storeFile = file(storeFilePath)
                storePassword = storePass
                keyAlias = alias
                keyPassword = keyPass
            }
        }
    }

    buildTypes {
        release {
            signingConfig = signingConfigs.getByName("release")

            isMinifyEnabled = true
            isShrinkResources = true

            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }

        debug {
            isMinifyEnabled = false
            isShrinkResources = false
        }
    }

    packaging {
        resources {
            excludes += setOf(
                "kotlin/**",
                "META-INF/version-control-info.textproto",
                "META-INF/com/android/build/gradle/app-metadata.properties"
            )
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}