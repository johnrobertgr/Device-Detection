/* *********************************************************************
 * This Source Code Form is copyright of 51Degrees Mobile Experts Limited.
 * Copyright 2017 51Degrees Mobile Experts Limited, 5 Charlotte Close,
 * Caversham, Reading, Berkshire, United Kingdom RG4 7BY
 *
 * This Source Code Form is the subject of the following patents and patent
 * applications, owned by 51Degrees Mobile Experts Limited of 5 Charlotte
 * Close, Caversham, Reading, Berkshire, United Kingdom RG4 7BY:
 * European Patent No. 2871816;
 * European Patent Application No. 17184134.9;
 * United States Patent Nos. 9,332,086 and 9,350,823; and
 * United States Patent Application No. 15/686,066.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.
 *
 * If a copy of the MPL was not distributed with this file, You can obtain
 * one at http://mozilla.org/MPL/2.0/.
 *
 * This Source Code Form is "Incompatible With Secondary Licenses", as
 * defined by the Mozilla Public License, v. 2.0.
 ********************************************************************** */

using FiftyOne.Mobile.Detection.Provider.Interop;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace FiftyOne.UnitTests.API
{
    public abstract class TrieBase : Base
    {
        protected override void AssertProperties(IWrapper provider)
        {
            for (int i = 0; i < provider.AvailableProperties.Count - 1; i++)
            {
                Assert.IsTrue(String.CompareOrdinal(
                    provider.AvailableProperties[i],
                    provider.AvailableProperties[i + 1]) < 0);
            }

            base.AssertProperties(provider);
        }

        protected override IWrapper CreateWrapper()
        {
            Utils.CheckFileExists(DataFile);
            return new TrieWrapper(DataFile);
        }

        protected override IWrapper CreateWrapper(string properties)
        {
            Utils.CheckFileExists(DataFile);
            return new TrieWrapper(DataFile, properties);
        }

        protected override IWrapper CreateWrapper(string[] properties)
        {
            Utils.CheckFileExists(DataFile);
            return new TrieWrapper(DataFile, properties);
        }

        protected void TestDrift(IWrapper provider)
        {
            double total = 0;
            double correct = 0;
            foreach (var userAgent in UserAgentGenerator.GetUniqueUserAgents())
            {
                var alteredUserAgent = " " + userAgent;
                provider.Drift = 0;
                var match = provider.Match(userAgent);
                provider.Drift = 1;
                var alteredMatch = provider.Match(alteredUserAgent);

                if (match.getValue("Id").Equals(alteredMatch.getValue("Id")))
                {
                    correct++;
                }
                total++;
            }
            Console.WriteLine("'{0}' altered User-Agents of " +
                "'{1}' ({2:P2}) correctly identified.",
                correct,
                total,
                correct / total);
            Assert.IsTrue(correct / total > 0.8);
        }

        protected void TestDifference(IWrapper provider)
        {
            var userAgent = "Mozilla/5.0 (Linux; Android 4.4.2; GT-N7105 " +
                "Build/KOT49H) AppleWebKit/537.36 (KHTML, like Gecko) " + 
                "Chrome/32.0.1700.58 Mobile Safari/537.36";
            var alteredUserAgent = "Mozilla/5.0 (Linux; Android 4.4.1; " + 
                "GT-N7105 Build/KOT49H) AppleWebKit/537.36 (KHTML, like Gecko)" +
                " Chrome/32.0.1700.58 Mobile Safari/537.36";
            var match = provider.Match(userAgent);
            provider.Difference = 1;
            var alteredMatch = provider.Match(alteredUserAgent);
            Assert.AreEqual(match.getValue("Id"), alteredMatch.getValue("Id"));
        }
    }
}
